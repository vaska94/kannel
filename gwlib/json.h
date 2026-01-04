/*
 * json.h - Lightweight JSON parsing and generation for Kannel
 *
 * Copyright (c) 2026 https://github.com/vaska94/kannel
 *
 * JSON parsing derived from cJSON by Dave Gamble (MIT License)
 * https://github.com/DaveGamble/cJSON
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef JSON_H
#define JSON_H

#include "octstr.h"

/* Opaque JSON value type */
typedef struct JSON JSON;

/* JSON value types */
#define JSON_INVALID  0
#define JSON_FALSE    1
#define JSON_TRUE     2
#define JSON_NULL     3
#define JSON_NUMBER   4
#define JSON_STRING   5
#define JSON_ARRAY    6
#define JSON_OBJECT   7

/*
 * Parsing functions
 */

/* Parse JSON string into a JSON object. Returns NULL on error.
 * Caller must call json_destroy() to free the result. */
JSON *json_parse(Octstr *json);

/* Parse JSON from C string with explicit length */
JSON *json_parse_cstr(const char *json, long len);

/* Destroy a JSON object and free all memory */
void json_destroy(JSON *json);

/*
 * Type checking
 */

/* Get the type of a JSON value */
int json_type(const JSON *json);

/* Type check helpers */
int json_is_object(const JSON *json);
int json_is_array(const JSON *json);
int json_is_string(const JSON *json);
int json_is_number(const JSON *json);
int json_is_bool(const JSON *json);
int json_is_null(const JSON *json);

/*
 * Value extraction from objects
 */

/* Get a field from a JSON object. Returns NULL if not found or wrong type.
 * The returned JSON pointer is owned by the parent - do not destroy it. */
JSON *json_get(const JSON *json, const char *key);

/* Get string value. Returns new Octstr that caller must destroy, or NULL. */
Octstr *json_get_string(const JSON *json, const char *key);

/* Get integer value. Returns default_val if not found or not a number. */
long json_get_integer(const JSON *json, const char *key, long default_val);

/* Get double value. Returns default_val if not found or not a number. */
double json_get_double(const JSON *json, const char *key, double default_val);

/* Get boolean value. Returns -1 if not found, 0 for false, 1 for true. */
int json_get_bool(const JSON *json, const char *key);

/*
 * Direct value access (for non-object values or after json_get)
 */

/* Get string value from a JSON string node */
Octstr *json_value_string(const JSON *json);

/* Get integer value from a JSON number node */
long json_value_integer(const JSON *json);

/* Get double value from a JSON number node */
double json_value_double(const JSON *json);

/* Get boolean value from a JSON bool node (0 or 1) */
int json_value_bool(const JSON *json);

/*
 * Array access
 */

/* Get array size. Returns 0 if not an array. */
int json_array_size(const JSON *json);

/* Get array item by index. Returns NULL if out of bounds.
 * The returned JSON pointer is owned by the parent - do not destroy it. */
JSON *json_array_get(const JSON *json, int index);

/*
 * JSON output generation
 */

/* Escape a string for JSON output. Returns new Octstr that caller must destroy. */
Octstr *json_escape_string(Octstr *str);

/* Create a simple JSON object from key-value pairs.
 * Format string specifies types: s=string(Octstr*), S=string(char*),
 * i=integer(long), d=double, b=bool(int), n=null
 * Example: json_object("sis", "name", name_octstr, "count", 42, "active", flag_octstr)
 * Returns new Octstr that caller must destroy. */
Octstr *json_object(const char *fmt, ...);

/* Convenience: create single key-value response object.
 * json_response("status", 0, "message", "OK")
 * Returns: {"status":0,"message":"OK"}  */
Octstr *json_response(const char *key1, long val1, const char *key2, const char *val2);

#endif /* JSON_H */
