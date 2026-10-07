#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""json2c.py -- generate a cJSON-based C encoder/decoder from a JSON file.

usage:
    python3 json2c.py input.json                  # writes input.h / input.c
    python3 json2c.py input.json -o mytypes      # writes mytypes.h / mytypes.c
    python3 json2c.py input.json --name config   # root type is config_t

The generated code depends only on cJSON (single-file cJSON.c + cJSON.h):
    https://github.com/DaveGamble/cJSON

How the JSON sample is mapped to C:
    object         struct <name>_t.  Nested objects are pointers, so a missing
                   object is simply NULL.
    array          <elem> *name; size_t name_count;  (heap allocated).
                   Arrays nested inside arrays use a generated helper struct
                   <elem>_array_t { <elem> *items; size_t count; }.
    string         char * (heap copy, released by <name>_free()).
    integer        int, or long long if the sample does not fit an int.
    other number   double.
    true / false   bool.
    null           treated like a missing value (all fields are optional).

Generated API for every struct type X_t:
    bool   X_from_json(const cJSON *json, X_t *out)    JSON  -> struct
    cJSON *X_to_json(const X_t *obj)                   struct -> JSON
    void   X_free(X_t *obj)                            free owned heap data
For the root type there are additional convenience helpers:
    object root:  X_parse(text), X_serialize(obj, pretty)
    array  root:  X_from_json/to_json/free for the whole array plus
                  X_parse(text, &count), X_serialize(items, count, pretty)

Decoding is deliberately lenient: unknown members are ignored, missing
members keep their zero/NULL value and array elements of an unexpected
type are skipped.

Caveat: cJSON stores every number in a double, so integers outside the
exact range of double (+/-2^53) may lose precision.
"""

import argparse
import json
import os
import re
import sys


# ---------------------------------------------------------------------------
# identifiers, string literals, warnings
# ---------------------------------------------------------------------------

C_KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while",
}

WARNINGS = []


def warn(message):
    WARNINGS.append(message)
    sys.stderr.write("json2c: warning: %s\n" % message)


def sanitize_ident(name, fallback="field"):
    """Turn an arbitrary JSON key into a valid C identifier."""
    s = re.sub(r"[^0-9A-Za-z_]", "_", str(name))
    s = re.sub(r"_+", "_", s).strip("_")
    if not s:
        s = fallback
    if s[0].isdigit():
        s = "n_" + s
    if s in C_KEYWORDS:
        s += "_"
    return s


def unique_name(name, taken):
    if name not in taken:
        taken.add(name)
        return name
    n = 2
    while "%s_%d" % (name, n) in taken:
        n += 1
    chosen = "%s_%d" % (name, n)
    taken.add(chosen)
    return chosen


def c_literal(text):
    """Encode a Python string as a C string literal (UTF-8 aware)."""
    parts = ['"']
    for byte in text.encode("utf-8"):
        if byte == 0x22:
            parts.append('\\"')
        elif byte == 0x5C:
            parts.append("\\\\")
        elif 0x20 <= byte <= 0x7E:
            parts.append(chr(byte))
        else:
            parts.append("\\%03o" % byte)
    parts.append('"')
    return "".join(parts)


def comment_safe(text):
    """Make a string safe to embed inside a C /* ... */ comment."""
    return text.replace("*/", "* /")


# ---------------------------------------------------------------------------
# C type model
# ---------------------------------------------------------------------------

class CScalar(object):
    def __init__(self, base, star, cname, jname):
        self.base = base
        self.star = star
        self.cname = cname
        self.jname = jname

    def key(self):
        return self.base


C_INT = CScalar("int", "", "int", "integer")
C_LONG = CScalar("long long", "", "long_long", "integer (long long)")
C_DOUBLE = CScalar("double", "", "double", "number")
C_BOOL = CScalar("bool", "", "bool", "boolean")
C_STRING = CScalar("char", "*", "string", "string")

NUMERIC = (C_INT, C_LONG, C_DOUBLE)


class CStruct(object):
    def __init__(self, struct):
        self.struct = struct

    def key(self):
        return "struct:" + self.struct.name


class CArray(object):
    def __init__(self, elem):
        self.elem = elem

    def key(self):
        return "array:" + self.elem.key()


def elem_ctype(elem):
    """The C type of ONE array element (e.g. "int", "char *", "foo_t")."""
    if isinstance(elem, CStruct):
        return elem.struct.name + "_t"
    return elem.base + (" *" if elem.star else "")


def array_decl(elem, name):
    """Declaration of an array variable (pointer to the element type)."""
    et = elem_ctype(elem)
    if et.endswith("*"):
        return et + "*" + name
    return et + " *" + name


def elem_cast(elem):
    et = elem_ctype(elem)
    if et.endswith("*"):
        return "(%s*)" % et
    return "(%s *)" % et


def decl(type_, name):
    """Declaration of a struct member / parameter of the given type."""
    if isinstance(type_, CStruct):
        return "%s_t *%s" % (type_.struct.name, name)
    if isinstance(type_, CArray):
        return array_decl(type_.elem, name)
    return "%s %s%s" % (type_.base, type_.star, name)


def type_jname(type_):
    if isinstance(type_, CArray):
        return "array of %s" % type_jname(type_.elem)
    if isinstance(type_, CStruct):
        return "object (%s_t)" % type_.struct.name
    return type_.jname


def type_cname(type_):
    if isinstance(type_, CArray):
        return type_cname(type_.elem) + "_array"
    if isinstance(type_, CStruct):
        return type_.struct.name
    return type_.cname


# ---------------------------------------------------------------------------
# struct model
# ---------------------------------------------------------------------------

class Field(object):
    def __init__(self, json_name, c_name, count_name, type_):
        self.json_name = json_name
        self.json_lit = c_literal(json_name)
        self.c_name = c_name
        self.count_name = count_name
        self.type = type_


class Struct(object):
    def __init__(self, name):
        self.name = name
        self.fields = []
        self.used_members = set()
        self.wrapper_elem = None   # element type for generated array wrappers


class Context(object):
    def __init__(self):
        self.structs = []          # emitted in this order (children first)
        self.taken_names = set()
        self.wrappers = {}

    def reserve(self, base):
        return unique_name(sanitize_ident(base, "type"), self.taken_names)


# ---------------------------------------------------------------------------
# type inference
# ---------------------------------------------------------------------------

def scalar_type(value):
    if isinstance(value, bool):
        return C_BOOL
    if isinstance(value, int):
        if -(2 ** 31) <= value <= 2 ** 31 - 1:
            return C_INT
        if -(2 ** 63) <= value <= 2 ** 63 - 1:
            return C_LONG
        warn("integer %s does not fit into 64 bits, using double" % value)
        return C_DOUBLE
    if isinstance(value, float):
        return C_DOUBLE
    return C_STRING


def merge_scalars(a, b, hint):
    if a is b:
        return a
    if a in NUMERIC and b in NUMERIC:
        if C_DOUBLE in (a, b):
            return C_DOUBLE
        if C_LONG in (a, b):
            return C_LONG
        return C_INT
    warn('"%s" has mixed element types (%s vs %s); using %s'
         % (hint, a.jname, b.jname, a.jname))
    return a


def infer_value(samples, hint, ctx):
    """Infer the C type for a JSON value from one or more sample values."""
    present = [s for s in samples if s is not None]
    if not present:
        warn('"%s" only contains null values; using an optional char *' % hint)
        return C_STRING
    if all(isinstance(v, dict) for v in present):
        return build_struct(present, hint, ctx)
    if all(isinstance(v, list) for v in present):
        elements = [e for v in present for e in v]
        if not elements:
            warn('"%s" contains only empty arrays; assuming int elements' % hint)
            return CArray(C_INT)
        return CArray(infer_value(elements, hint + "[]", ctx))
    if any(isinstance(v, (dict, list)) for v in present):
        warn('"%s" mixes objects/arrays with scalars; '
             "using the type of the first value" % hint)
        return infer_value([present[0]], hint, ctx)
    type_ = scalar_type(present[0])
    for value in present[1:]:
        other = scalar_type(value)
        if other is not type_:
            type_ = merge_scalars(type_, other, hint)
    return type_


def build_struct(samples, hint, ctx, name=None):
    """Create a Struct from one or more sample objects (fields are merged)."""
    if name is None:
        struct_name = ctx.reserve(hint)
    else:
        struct_name = unique_name(sanitize_ident(name), ctx.taken_names)
    struct = Struct(struct_name)

    keys = []
    for sample in samples:
        for key in sample:
            if key not in keys:
                keys.append(key)

    for key in keys:
        sub = [sample[key] for sample in samples if key in sample]
        field_type = infer_value(sub, key, ctx)
        if isinstance(field_type, CArray):
            field_type = CArray(array_elem_type(field_type.elem, ctx))
        c_name = unique_name(sanitize_ident(key), struct.used_members)
        count_name = None
        if isinstance(field_type, CArray):
            count_name = unique_name(c_name + "_count", struct.used_members)
        struct.fields.append(Field(key, c_name, count_name, field_type))

    ctx.structs.append(struct)
    return CStruct(struct)


def array_elem_type(elem, ctx):
    """Element type as it is stored inside a struct member.

    Plain elements stay as they are.  An element that is itself an array
    cannot be stored as a pointer+count pair, so it is wrapped in a
    generated "<elem>_array_t" struct instead.
    """
    if not isinstance(elem, CArray):
        return elem
    known = ctx.wrappers.get(elem.key())
    if known is not None:
        return known
    inner = array_elem_type(elem.elem, ctx)
    wrapper = Struct(ctx.reserve(type_cname(elem.elem) + "_array"))
    wrapper.wrapper_elem = inner
    ctx.structs.append(wrapper)
    wrapped = CStruct(wrapper)
    ctx.wrappers[elem.key()] = wrapped
    return wrapped


# ---------------------------------------------------------------------------
# C code generation
# ---------------------------------------------------------------------------

class Generator(object):
    def __init__(self, ctx, prefix, src_name, root_kind, root_type, root_name):
        self.ctx = ctx
        self.prefix = prefix
        self.src_name = src_name
        self.root_kind = root_kind
        self.root_type = root_type
        self.root_name = root_name
        guard = re.sub(r"[^0-9A-Za-z]", "_", prefix).upper() + "_H"
        if guard[0].isdigit():
            guard = "_" + guard
        self.guard = guard

    # ----- header ----------------------------------------------------------

    def header(self):
        L = []
        L.append("/*")
        L.append(" * %s.h" % os.path.basename(self.prefix + ".h"))
        L.append(" * Generated by json2c.py from %s -- do not edit by hand."
                 % self.src_name)
        L.append(" *")
        L.append(" * Requires cJSON: https://github.com/DaveGamble/cJSON")
        L.append(" *")
        L.extend(self.usage_comment())
        L.append(" */")
        L.append("")
        L.append("#ifndef %s" % self.guard)
        L.append("#define %s" % self.guard)
        L.append("")
        L.append("#include <stdbool.h>")
        L.append("#include <stddef.h>")
        L.append("")
        L.append('#include "cJSON.h"')
        L.append("")
        for struct in self.ctx.structs:
            L.extend(self.struct_definition(struct))
            L.append("")
        for struct in self.ctx.structs:
            L.extend(self.api_declarations(struct))
            L.append("")
        L.extend(self.root_api_declarations())
        L.append("#endif /* %s */" % self.guard)
        L.append("")
        return "\n".join(L)

    def usage_comment(self):
        L = [" * Quick usage:"]
        if self.root_kind == "object":
            r = self.root_name
            L += [
                " *     %s_t *obj = %s_parse(json_text);" % (r, r),
                " *     if (obj == NULL) { /* parse error */ }",
                " *     ... use obj->field ...",
                " *     char *text = %s_serialize(obj, true);" % r,
                " *     free(text);",
                " *     %s_free(obj);   /* releases owned members */" % r,
                " *     free(obj);",
            ]
        else:
            r = self.root_name
            L += [
                " *     size_t n = 0;",
                " *     %s = %s_parse(json_text, &n);"
                % (array_decl(self.root_type, "items"), r),
                " *     if (items == NULL) { /* parse error */ }",
                " *     ... items[i].field ...",
                " *     %s_free(items, n);" % r,
            ]
        return L

    def struct_definition(self, struct):
        L = []
        if struct.wrapper_elem is not None:
            L.append("/* helper struct for nested arrays */")
            rows = [
                (array_decl(struct.wrapper_elem, "items"), "the array elements"),
                ("size_t count", "number of elements in items"),
            ]
        else:
            L.append("/* struct: %s */" % struct.name)
            rows = []
            for f in struct.fields:
                if isinstance(f.type, CArray):
                    rows.append((decl(f.type, f.c_name),
                                 "JSON %s: %s" % (f.json_lit, type_jname(f.type))))
                    rows.append(("size_t %s" % f.count_name,
                                 "number of elements in %s" % f.c_name))
                else:
                    rows.append((decl(f.type, f.c_name),
                                 "JSON %s: %s" % (f.json_lit, type_jname(f.type))))
        if not rows:
            rows = [("char _empty_", "placeholder: object had no fields")]
        width = max(len(d) for d, _ in rows) + 1
        L.append("typedef struct %s {" % struct.name)
        for d, c in rows:
            L.append("    %-*s/* %s */" % (width, d + ";", comment_safe(c)))
        L.append("} %s_t;" % struct.name)
        return L

    def api_declarations(self, struct):
        n = struct.name
        t = n + "_t"
        first = ("/* decode a JSON array node into *out */"
                 if struct.wrapper_elem is not None
                 else "/* decode a JSON object node into *out */")
        return [
            first,
            "bool %s_from_json(const cJSON *json, %s *out);" % (n, t),
            "",
            "/* encode *obj into a new cJSON tree (caller must delete it) */",
            "cJSON *%s_to_json(const %s *obj);" % (n, t),
            "",
            "/* release all heap data owned by *obj (obj itself is not freed) */",
            "void %s_free(%s *obj);" % (n, t),
        ]

    def root_api_declarations(self):
        L = []
        if self.root_kind == "object":
            r = self.root_name
            L.append("/* ---- convenience helpers for the root type ---- */")
            L.append("")
            L.append("/* parse a JSON string into a heap object (free with free()) */")
            L.append("%s_t *%s_parse(const char *text);" % (r, r))
            L.append("")
            L.append("/* serialize to a NUL-terminated string (free with free()) */")
            L.append("char *%s_serialize(const %s_t *obj, bool pretty);" % (r, r))
        else:
            r = self.root_name
            e = self.root_type
            L.append("/* ---- the root type is a JSON array of %s ---- */"
                     % comment_safe(type_jname(e)))
            L.append("")
            L.append("/* decode a JSON array; *out_items is a heap array */")
            L.append("bool %s_from_json(const cJSON *json, %s, size_t *out_count);"
                     % (r, array_decl(e, "*out_items")))
            L.append("")
            L.append("cJSON *%s_to_json(%s, size_t count);"
                     % (r, array_decl(e, "items")))
            L.append("")
            L.append("/* free the item array and everything it owns */")
            L.append("void %s_free(%s, size_t count);" % (r, array_decl(e, "items")))
            L.append("")
            L.append("/* parse a JSON string into a heap array (free with %s_free) */" % r)
            L.append("%s(const char *text, size_t *out_count);"
                     % array_decl(e, r + "_parse"))
            L.append("")
            L.append("/* serialize the array to a NUL-terminated string */")
            L.append("char *%s_serialize(%s, size_t count, bool pretty);"
                     % (r, array_decl(e, "items")))
        return L

    # ----- source ----------------------------------------------------------

    def source(self):
        L = []
        L.append("/*")
        L.append(" * %s.c" % os.path.basename(self.prefix + ".c"))
        L.append(" * Generated by json2c.py from %s -- do not edit by hand."
                 % self.src_name)
        L.append(" *")
        L.append(" * Note: cJSON stores every number in a double; integers")
        L.append(" * outside +/-2^53 may lose precision.")
        if WARNINGS:
            L.append(" *")
            L.append(" * Assumptions made while analysing the sample:")
            for w in WARNINGS:
                L.append(" *   - %s" % comment_safe(w))
        L.append(" */")
        L.append("")
        L.append("#include <stdlib.h>")
        L.append("#include <string.h>")
        L.append("")
        L.append('#include "%s.h"' % os.path.basename(self.prefix))
        L.append("")
        L.append("/* strdup() is not standard C, so use our own copy */")
        L.append("static char *jc_strdup(const char *s)")
        L.append("{")
        L.append("    size_t n;")
        L.append("    char *p;")
        L.append("")
        L.append("    if (s == NULL) {")
        L.append("        return NULL;")
        L.append("    }")
        L.append("    n = strlen(s) + 1;")
        L.append("    p = (char *)malloc(n);")
        L.append("    if (p != NULL) {")
        L.append("        memcpy(p, s, n);")
        L.append("    }")
        L.append("    return p;")
        L.append("}")
        L.append("")
        for struct in self.ctx.structs:
            L.extend(self.decode_function(struct))
            L.append("")
            L.extend(self.encode_function(struct))
            L.append("")
            L.extend(self.free_function(struct))
            L.append("")
        L.extend(self.root_helpers())
        return "\n".join(L)

    # ----- decoding --------------------------------------------------------

    def array_foreach_decode(self, elem, src, dest, count, ind):
        """The cJSON_ArrayForEach loop that decodes array elements."""
        p = "    " * ind
        L = ["%scJSON_ArrayForEach(el, %s) {" % (p, src)]
        if isinstance(elem, CStruct):
            s = elem.struct
            check = ("cJSON_IsArray(el)" if s.wrapper_elem is not None
                     else "cJSON_IsObject(el)")
            L.append("%s    if (%s) {" % (p, check))
            L.append("%s        %s_t *slot = &%s[%s];" % (p, s.name, dest, count))
            L.append("")
            L.append("%s        if (!%s_from_json(el, slot)) {" % (p, s.name))
            L.append("%s            goto fail;" % p)
            L.append("%s        }" % p)
            L.append("%s        %s++;" % (p, count))
            L.append("%s    }" % p)
        elif elem is C_STRING:
            L.append("%s    if (cJSON_IsString(el)) {" % p)
            L.append("%s        char *v = jc_strdup(el->valuestring);" % p)
            L.append("")
            L.append("%s        if (v == NULL) {" % p)
            L.append("%s            goto fail;" % p)
            L.append("%s        }" % p)
            L.append("%s        %s[%s++] = v;" % (p, dest, count))
            L.append("%s    }" % p)
        elif elem is C_BOOL:
            L.append("%s    if (cJSON_IsBool(el)) {" % p)
            L.append("%s        %s[%s++] = cJSON_IsTrue(el);" % (p, dest, count))
            L.append("%s    }" % p)
        else:
            cast = ""
            if elem is C_INT:
                cast = "(int)"
            elif elem is C_LONG:
                cast = "(long long)"
            L.append("%s    if (cJSON_IsNumber(el)) {" % p)
            L.append("%s        %s[%s++] = %sel->valuedouble;" % (p, dest, count, cast))
            L.append("%s    }" % p)
        L.append("%s}" % p)
        return L

    def field_decode_lines(self, f):
        L = ["    /* %s: %s */" % (comment_safe(f.json_lit),
                                   comment_safe(type_jname(f.type)))]
        L.append("    item = cJSON_GetObjectItemCaseSensitive(json, %s);" % f.json_lit)
        t = f.type
        if isinstance(t, CStruct):
            s = t.struct
            L.append("    if (cJSON_IsObject(item)) {")
            L.append("        out->%s = (%s_t *)calloc(1, sizeof(%s_t));"
                     % (f.c_name, s.name, s.name))
            L.append("        if (out->%s == NULL) {" % f.c_name)
            L.append("            goto fail;")
            L.append("        }")
            L.append("        if (!%s_from_json(item, out->%s)) {" % (s.name, f.c_name))
            L.append("            goto fail;")
            L.append("        }")
            L.append("    }")
        elif isinstance(t, CArray):
            elem = t.elem
            dest = "out->%s" % f.c_name
            count = "out->%s" % f.count_name
            L.append("    if (cJSON_IsArray(item)) {")
            L.append("        size_t n = (size_t)cJSON_GetArraySize(item);")
            L.append("        const cJSON *el = NULL;")
            L.append("")
            L.append("        %s = %scalloc(n > 0 ? n : 1, sizeof(%s));"
                     % (dest, elem_cast(elem), elem_ctype(elem)))
            L.append("        if (%s == NULL) {" % dest)
            L.append("            goto fail;")
            L.append("        }")
            L.extend(self.array_foreach_decode(elem, "item", dest, count, 2))
            L.append("    }")
        elif t is C_STRING:
            L.append("    if (cJSON_IsString(item)) {")
            L.append("        out->%s = jc_strdup(item->valuestring);" % f.c_name)
            L.append("        if (out->%s == NULL) {" % f.c_name)
            L.append("            goto fail;")
            L.append("        }")
            L.append("    }")
        elif t is C_BOOL:
            L.append("    if (cJSON_IsBool(item)) {")
            L.append("        out->%s = cJSON_IsTrue(item);" % f.c_name)
            L.append("    }")
        else:
            cast = ""
            if t is C_INT:
                cast = "(int)"
            elif t is C_LONG:
                cast = "(long long)"
            L.append("    if (cJSON_IsNumber(item)) {")
            L.append("        out->%s = %sitem->valuedouble;" % (f.c_name, cast))
            L.append("    }")
        L.append("")
        return L

    def decode_function(self, struct):
        n = struct.name
        L = ["bool %s_from_json(const cJSON *json, %s_t *out)" % (n, n), "{"]
        if struct.wrapper_elem is not None:
            elem = struct.wrapper_elem
            L += [
                "    const cJSON *el = NULL;",
                "    size_t n;",
                "",
                "    if (json == NULL || out == NULL || !cJSON_IsArray(json)) {",
                "        return false;",
                "    }",
                "    memset(out, 0, sizeof(*out));",
                "    n = (size_t)cJSON_GetArraySize(json);",
                "    out->items = %scalloc(n > 0 ? n : 1, sizeof(%s));"
                % (elem_cast(elem), elem_ctype(elem)),
                "    if (out->items == NULL) {",
                "        goto fail;",
                "    }",
            ]
            L.extend(self.array_foreach_decode(elem, "json", "out->items",
                                               "out->count", 1))
        else:
            L += [
                "    const cJSON *item = NULL;",
                "",
                "    if (json == NULL || out == NULL || !cJSON_IsObject(json)) {",
                "        return false;",
                "    }",
                "    memset(out, 0, sizeof(*out));",
                "",
            ]
            for f in struct.fields:
                L.extend(self.field_decode_lines(f))
        L.append("    return true;")
        if any("goto fail" in line for line in L):
            L += ["", "fail:", "    %s_free(out);" % n, "    return false;"]
        L.append("}")
        return L

    # ----- encoding --------------------------------------------------------

    def array_encode_loop(self, elem, items, count, arr, ind, fail_lines):
        p = "    " * ind
        L = ["%sfor (size_t i = 0; i < %s; i++) {" % (p, count)]
        if isinstance(elem, CStruct):
            create = "%s_to_json(&%s[i])" % (elem.struct.name, items)
        elif elem is C_STRING:
            create = 'cJSON_CreateString(%s[i] != NULL ? %s[i] : "")' % (items, items)
        elif elem is C_BOOL:
            create = "cJSON_CreateBool(%s[i])" % items
        elif elem is C_DOUBLE:
            create = "cJSON_CreateNumber(%s[i])" % items
        else:
            create = "cJSON_CreateNumber((double)%s[i])" % items
        L.append("%s    cJSON *el = %s;" % (p, create))
        L.append("")
        L.append("%s    if (el == NULL) {" % p)
        for line in fail_lines:
            L.append("%s        %s" % (p, line))
        L.append("%s    }" % p)
        L.append("%s    cJSON_AddItemToArray(%s, el);" % (p, arr))
        L.append("%s}" % p)
        return L

    def field_encode_lines(self, f):
        t = f.type
        L = ["    /* %s: %s */" % (comment_safe(f.json_lit),
                                   comment_safe(type_jname(t)))]
        if isinstance(t, CStruct):
            s = t.struct
            L.append("    if (obj->%s != NULL) {" % f.c_name)
            L.append("        cJSON *val = %s_to_json(obj->%s);" % (s.name, f.c_name))
            L.append("")
            L.append("        if (val == NULL) {")
            L.append("            goto fail;")
            L.append("        }")
            L.append("        cJSON_AddItemToObject(json, %s, val);" % f.json_lit)
            L.append("    }")
        elif isinstance(t, CArray):
            elem = t.elem
            items = "obj->%s" % f.c_name
            count = "obj->%s" % f.count_name
            L.append("    if (%s != NULL) {" % items)
            L.append("        cJSON *val = cJSON_CreateArray();")
            L.append("")
            L.append("        if (val == NULL) {")
            L.append("            goto fail;")
            L.append("        }")
            L.extend(self.array_encode_loop(elem, items, count, "val", 2,
                                            ["cJSON_Delete(val);", "goto fail;"]))
            L.append("        cJSON_AddItemToObject(json, %s, val);" % f.json_lit)
            L.append("    }")
        elif t is C_STRING:
            L.append("    if (obj->%s != NULL) {" % f.c_name)
            L.append("        cJSON *val = cJSON_CreateString(obj->%s);" % f.c_name)
            L.append("")
            L.append("        if (val == NULL) {")
            L.append("            goto fail;")
            L.append("        }")
            L.append("        cJSON_AddItemToObject(json, %s, val);" % f.json_lit)
            L.append("    }")
        else:
            if t is C_BOOL:
                create = "cJSON_CreateBool(obj->%s)" % f.c_name
            elif t is C_DOUBLE:
                create = "cJSON_CreateNumber(obj->%s)" % f.c_name
            else:
                create = "cJSON_CreateNumber((double)obj->%s)" % f.c_name
            L.append("    {")
            L.append("        cJSON *val = %s;" % create)
            L.append("")
            L.append("        if (val == NULL) {")
            L.append("            goto fail;")
            L.append("        }")
            L.append("        cJSON_AddItemToObject(json, %s, val);" % f.json_lit)
            L.append("    }")
        L.append("")
        return L

    def encode_function(self, struct):
        n = struct.name
        L = ["cJSON *%s_to_json(const %s_t *obj)" % (n, n), "{"]
        L += [
            "    cJSON *json = NULL;",
            "",
            "    if (obj == NULL) {",
            "        return NULL;",
            "    }",
        ]
        if struct.wrapper_elem is not None:
            L += [
                "    json = cJSON_CreateArray();",
                "    if (json == NULL) {",
                "        return NULL;",
                "    }",
            ]
            L.extend(self.array_encode_loop(struct.wrapper_elem, "obj->items",
                                            "obj->count", "json", 1,
                                            ["cJSON_Delete(json);", "return NULL;"]))
        else:
            L += [
                "    json = cJSON_CreateObject();",
                "    if (json == NULL) {",
                "        return NULL;",
                "    }",
                "",
            ]
            for f in struct.fields:
                L.extend(self.field_encode_lines(f))
        L.append("    return json;")
        if any("goto fail" in line for line in L):
            L += ["", "fail:", "    cJSON_Delete(json);", "    return NULL;"]
        L.append("}")
        return L

    # ----- freeing ---------------------------------------------------------

    def array_free_loop(self, elem, items, count, ind):
        p = "    " * ind
        if isinstance(elem, CStruct):
            return ["%sfor (size_t i = 0; i < %s; i++) {" % (p, count),
                    "%s    %s_free(&%s[i]);" % (p, elem.struct.name, items),
                    "%s}" % p]
        if elem is C_STRING:
            return ["%sfor (size_t i = 0; i < %s; i++) {" % (p, count),
                    "%s    free(%s[i]);" % (p, items),
                    "%s}" % p]
        return []

    def field_free_lines(self, f):
        t = f.type
        if isinstance(t, CStruct):
            return ["    if (obj->%s != NULL) {" % f.c_name,
                    "        %s_free(obj->%s);" % (t.struct.name, f.c_name),
                    "        free(obj->%s);" % f.c_name,
                    "    }"]
        if isinstance(t, CArray):
            lines = self.array_free_loop(t.elem, "obj->%s" % f.c_name,
                                         "obj->%s" % f.count_name, 1)
            lines.append("    free(obj->%s);" % f.c_name)
            return lines
        if t is C_STRING:
            return ["    free(obj->%s);" % f.c_name]
        return []

    def free_function(self, struct):
        n = struct.name
        L = ["void %s_free(%s_t *obj)" % (n, n), "{"]
        L += ["    if (obj == NULL) {", "        return;", "    }"]
        if struct.wrapper_elem is not None:
            L.extend(self.array_free_loop(struct.wrapper_elem, "obj->items",
                                          "obj->count", 1))
            L.append("    free(obj->items);")
        else:
            for f in struct.fields:
                L.extend(self.field_free_lines(f))
        L.append("    memset(obj, 0, sizeof(*obj));")
        L.append("}")
        return L

    # ----- root helpers ----------------------------------------------------

    def root_helpers(self):
        if self.root_kind == "object":
            return self.root_object_helpers()
        return self.root_array_helpers()

    def root_object_helpers(self):
        r = self.root_name
        t = r + "_t"
        return [
            "%s_t *%s_parse(const char *text)" % (r, r),
            "{",
            "    cJSON *json = NULL;",
            "    %s_t *obj = NULL;" % r,
            "",
            "    if (text == NULL) {",
            "        return NULL;",
            "    }",
            "    json = cJSON_Parse(text);",
            "    if (json == NULL) {",
            "        return NULL;",
            "    }",
            "    obj = (%s_t *)calloc(1, sizeof(%s_t));" % (t, t),
            "    if (obj == NULL || !%s_from_json(json, obj)) {" % r,
            "        free(obj);",
            "        cJSON_Delete(json);",
            "        return NULL;",
            "    }",
            "    cJSON_Delete(json);",
            "    return obj;",
            "}",
            "",
            "char *%s_serialize(const %s_t *obj, bool pretty)" % (r, t),
            "{",
            "    cJSON *json = NULL;",
            "    char *text = NULL;",
            "",
            "    if (obj == NULL) {",
            "        return NULL;",
            "    }",
            "    json = %s_to_json(obj);" % r,
            "    if (json == NULL) {",
            "        return NULL;",
            "    }",
            "    text = pretty ? cJSON_Print(json) : cJSON_PrintUnformatted(json);",
            "    cJSON_Delete(json);",
            "    return text;",
            "}",
        ]

    def root_array_helpers(self):
        r = self.root_name
        elem = self.root_type
        L = []
        L.append("bool %s_from_json(const cJSON *json, %s, size_t *out_count)"
                 % (r, array_decl(elem, "*out_items")))
        L.append("{")
        L.append("    %s = NULL;" % array_decl(elem, "items"))
        L.append("    size_t count = 0;")
        L.append("    const cJSON *el = NULL;")
        L.append("")
        L.append("    if (json == NULL || out_items == NULL || out_count == NULL"
                 " || !cJSON_IsArray(json)) {")
        L.append("        return false;")
        L.append("    }")
        L.append("    {")
        L.append("        size_t n = (size_t)cJSON_GetArraySize(json);")
        L.append("")
        L.append("        items = %scalloc(n > 0 ? n : 1, sizeof(%s));"
                 % (elem_cast(elem), elem_ctype(elem)))
        L.append("        if (items == NULL) {")
        L.append("            return false;")
        L.append("        }")
        L.extend(self.array_foreach_decode(elem, "json", "items", "count", 2))
        L.append("    }")
        L.append("    *out_items = items;")
        L.append("    *out_count = count;")
        L.append("    return true;")
        L.append("")
        L.append("fail:")
        L.append("    %s_free(items, count);" % r)
        L.append("    return false;")
        L.append("}")
        L.append("")
        L.append("cJSON *%s_to_json(%s, size_t count)"
                 % (r, array_decl(elem, "items")))
        L.append("{")
        L.append("    cJSON *arr = cJSON_CreateArray();")
        L.append("")
        L.append("    if (arr == NULL) {")
        L.append("        return NULL;")
        L.append("    }")
        L.extend(self.array_encode_loop(elem, "items", "count", "arr", 1,
                                        ["cJSON_Delete(arr);", "return NULL;"]))
        L.append("    return arr;")
        L.append("}")
        L.append("")
        L.append("void %s_free(%s, size_t count)" % (r, array_decl(elem, "items")))
        L.append("{")
        L.append("    if (items == NULL) {")
        L.append("        return;")
        L.append("    }")
        loop = self.array_free_loop(elem, "items", "count", 1)
        L.extend(loop)
        if not loop:
            L.append("    (void)count;")
        L.append("    free(items);")
        L.append("}")
        L.append("")
        L.append("%s(const char *text, size_t *out_count)"
                 % array_decl(elem, r + "_parse"))
        L.append("{")
        L.append("    cJSON *json = NULL;")
        L.append("    %s = NULL;" % array_decl(elem, "items"))
        L.append("")
        L.append("    if (text == NULL || out_count == NULL) {")
        L.append("        return NULL;")
        L.append("    }")
        L.append("    json = cJSON_Parse(text);")
        L.append("    if (json == NULL) {")
        L.append("        return NULL;")
        L.append("    }")
        L.append("    if (!%s_from_json(json, &items, out_count)) {" % r)
        L.append("        cJSON_Delete(json);")
        L.append("        return NULL;")
        L.append("    }")
        L.append("    cJSON_Delete(json);")
        L.append("    return items;")
        L.append("}")
        L.append("")
        L.append("char *%s_serialize(%s, size_t count, bool pretty)"
                 % (r, array_decl(elem, "items")))
        L.append("{")
        L.append("    cJSON *arr = NULL;")
        L.append("    char *text = NULL;")
        L.append("")
        L.append("    arr = %s_to_json(items, count);" % r)
        L.append("    if (arr == NULL) {")
        L.append("        return NULL;")
        L.append("    }")
        L.append("    text = pretty ? cJSON_Print(arr) : cJSON_PrintUnformatted(arr);")
        L.append("    cJSON_Delete(arr);")
        L.append("    return text;")
        L.append("}")
        return L


# ---------------------------------------------------------------------------
# entry point
# ---------------------------------------------------------------------------

def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="json2c",
        description="Generate a C encoder/decoder (based on cJSON) from a "
                    "JSON file.")
    parser.add_argument("input", help="JSON sample file ('-' reads stdin)")
    parser.add_argument("-o", "--output", metavar="PREFIX",
                        help="output file prefix (default: the input file "
                             "name without extension)")
    parser.add_argument("--name", metavar="NAME",
                        help="name of the root type (default: derived from "
                             "the file name)")
    args = parser.parse_args(argv)

    if args.input == "-":
        src_name = "<stdin>"
        try:
            text = sys.stdin.read()
        except OSError as exc:
            parser.error(str(exc))
    else:
        src_name = args.input
        try:
            with open(args.input, "r", encoding="utf-8") as fp:
                text = fp.read()
        except OSError as exc:
            parser.error("cannot read %s: %s" % (args.input, exc))

    try:
        data = json.loads(text)
    except ValueError as exc:
        parser.error("%s is not valid JSON: %s" % (src_name, exc))

    if not isinstance(data, (dict, list)):
        parser.error("the top-level JSON value must be an object or an array")

    if args.input == "-":
        base = "stdin"
    else:
        base = os.path.splitext(os.path.basename(args.input))[0]
    prefix = args.output or base or "generated"
    root_name = args.name or base or "root"

    ctx = Context()
    root_cname = sanitize_ident(root_name, "root")

    if isinstance(data, dict):
        root_kind = "object"
        root_type = build_struct([data], root_cname, ctx, name=root_cname)
    else:
        root_kind = "array"
        ctx.taken_names.add(root_cname)      # reserved for the array API
        elem = infer_value(data, root_cname + "_item", ctx)
        root_type = array_elem_type(elem, ctx)

    gen = Generator(ctx, prefix, src_name, root_kind, root_type, root_cname)

    out_dir = os.path.dirname(prefix)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    header_path = prefix + ".h"
    source_path = prefix + ".c"
    with open(header_path, "w", encoding="utf-8") as fp:
        fp.write(gen.header())
    with open(source_path, "w", encoding="utf-8") as fp:
        fp.write(gen.source())

    print("wrote %s and %s (%d struct type%s generated)"
          % (header_path, source_path,
             len(ctx.structs), "" if len(ctx.structs) == 1 else "s"))
    return 0


if __name__ == "__main__":
    sys.exit(main())

