/*
 * check_json.c - test JSON parsing and generation
 *
 * Copyright (c) 2026 https://github.com/vaska94/kannel
 */

#include <string.h>
#include "gwlib/gwlib.h"

static void test_parse_object(void)
{
    Octstr *json_str;
    JSON *json;
    Octstr *str_val;
    long int_val;
    double dbl_val;
    int bool_val;

    json_str = octstr_create("{\"name\":\"test\",\"count\":42,\"pi\":3.14159,\"active\":true,\"empty\":null}");
    json = json_parse(json_str);

    if (json == NULL)
        panic(0, "check_json: failed to parse simple object");

    if (!json_is_object(json))
        panic(0, "check_json: parsed value is not an object");

    /* Test string extraction */
    str_val = json_get_string(json, "name");
    if (str_val == NULL)
        panic(0, "check_json: failed to get string field 'name'");
    if (octstr_compare(str_val, octstr_imm("test")) != 0)
        panic(0, "check_json: string field 'name' has wrong value");
    octstr_destroy(str_val);

    /* Test integer extraction */
    int_val = json_get_integer(json, "count", -1);
    if (int_val != 42)
        panic(0, "check_json: integer field 'count' has wrong value %ld", int_val);

    /* Test double extraction */
    dbl_val = json_get_double(json, "pi", 0.0);
    if (dbl_val < 3.14 || dbl_val > 3.15)
        panic(0, "check_json: double field 'pi' has wrong value %g", dbl_val);

    /* Test boolean extraction */
    bool_val = json_get_bool(json, "active");
    if (bool_val != 1)
        panic(0, "check_json: bool field 'active' should be true (1), got %d", bool_val);

    /* Test null detection */
    if (json_get_bool(json, "empty") != -1)
        panic(0, "check_json: null field should return -1 for bool");

    /* Test missing field */
    if (json_get_string(json, "missing") != NULL)
        panic(0, "check_json: missing field should return NULL");

    json_destroy(json);
    octstr_destroy(json_str);
}

static void test_parse_array(void)
{
    Octstr *json_str;
    JSON *json;
    JSON *item;
    int size;

    json_str = octstr_create("[1, 2, 3, \"four\", true]");
    json = json_parse(json_str);

    if (json == NULL)
        panic(0, "check_json: failed to parse array");

    if (!json_is_array(json))
        panic(0, "check_json: parsed value is not an array");

    size = json_array_size(json);
    if (size != 5)
        panic(0, "check_json: array size should be 5, got %d", size);

    /* Test first element */
    item = json_array_get(json, 0);
    if (!json_is_number(item))
        panic(0, "check_json: first array element should be a number");
    if (json_value_integer(item) != 1)
        panic(0, "check_json: first array element should be 1");

    /* Test string element */
    item = json_array_get(json, 3);
    if (!json_is_string(item))
        panic(0, "check_json: fourth array element should be a string");

    /* Test boolean element */
    item = json_array_get(json, 4);
    if (!json_is_bool(item))
        panic(0, "check_json: fifth array element should be a bool");

    /* Test out of bounds */
    item = json_array_get(json, 10);
    if (item != NULL)
        panic(0, "check_json: out of bounds should return NULL");

    json_destroy(json);
    octstr_destroy(json_str);
}

static void test_parse_nested(void)
{
    Octstr *json_str;
    JSON *json;
    JSON *nested;
    Octstr *val;

    json_str = octstr_create("{\"outer\":{\"inner\":\"value\"}}");
    json = json_parse(json_str);

    if (json == NULL)
        panic(0, "check_json: failed to parse nested object");

    nested = json_get(json, "outer");
    if (nested == NULL || !json_is_object(nested))
        panic(0, "check_json: failed to get nested object");

    val = json_get_string(nested, "inner");
    if (val == NULL || octstr_compare(val, octstr_imm("value")) != 0)
        panic(0, "check_json: failed to get nested value");

    octstr_destroy(val);
    json_destroy(json);
    octstr_destroy(json_str);
}

static void test_parse_escapes(void)
{
    Octstr *json_str;
    JSON *json;
    Octstr *val;

    /* Test escape sequences */
    json_str = octstr_create("{\"text\":\"line1\\nline2\\ttab\\\"\"}");
    json = json_parse(json_str);

    if (json == NULL)
        panic(0, "check_json: failed to parse escaped string");

    val = json_get_string(json, "text");
    if (val == NULL)
        panic(0, "check_json: failed to get escaped string");

    /* Should contain: line1\nline2\ttab" */
    if (octstr_search_char(val, '\n', 0) < 0)
        panic(0, "check_json: escaped newline not found");
    if (octstr_search_char(val, '\t', 0) < 0)
        panic(0, "check_json: escaped tab not found");
    if (octstr_search_char(val, '"', 0) < 0)
        panic(0, "check_json: escaped quote not found");

    octstr_destroy(val);
    json_destroy(json);
    octstr_destroy(json_str);
}

static void test_generate_object(void)
{
    Octstr *result;
    Octstr *name;

    name = octstr_create("test");
    result = json_object("siSb", "name", name, "count", 42L, "type", "foo", "active", 1);

    if (result == NULL)
        panic(0, "check_json: json_object returned NULL");

    /* Verify it's valid JSON by parsing it */
    JSON *json = json_parse(result);
    if (json == NULL)
        panic(0, "check_json: generated JSON is not valid: %s", octstr_get_cstr(result));

    /* Verify fields */
    Octstr *str_val = json_get_string(json, "name");
    if (str_val == NULL || octstr_compare(str_val, name) != 0)
        panic(0, "check_json: generated name field is wrong");
    octstr_destroy(str_val);

    long int_val = json_get_integer(json, "count", -1);
    if (int_val != 42)
        panic(0, "check_json: generated count field is wrong");

    json_destroy(json);
    octstr_destroy(result);
    octstr_destroy(name);
}

static void test_escape_string(void)
{
    Octstr *input;
    Octstr *escaped;

    input = octstr_create("hello\nworld\t\"quoted\"");
    escaped = json_escape_string(input);

    if (escaped == NULL)
        panic(0, "check_json: json_escape_string returned NULL");

    /* Should contain escaped sequences */
    if (octstr_search(escaped, octstr_imm("\\n"), 0) < 0)
        panic(0, "check_json: newline not escaped");
    if (octstr_search(escaped, octstr_imm("\\t"), 0) < 0)
        panic(0, "check_json: tab not escaped");
    if (octstr_search(escaped, octstr_imm("\\\""), 0) < 0)
        panic(0, "check_json: quote not escaped");

    octstr_destroy(escaped);
    octstr_destroy(input);
}

static void test_response(void)
{
    Octstr *result;
    JSON *json;

    result = json_response("status", 0, "message", "OK");
    if (result == NULL)
        panic(0, "check_json: json_response returned NULL");

    json = json_parse(result);
    if (json == NULL)
        panic(0, "check_json: json_response output is invalid JSON");

    if (json_get_integer(json, "status", -1) != 0)
        panic(0, "check_json: json_response status field wrong");

    Octstr *msg = json_get_string(json, "message");
    if (msg == NULL || octstr_compare(msg, octstr_imm("OK")) != 0)
        panic(0, "check_json: json_response message field wrong");

    octstr_destroy(msg);
    json_destroy(json);
    octstr_destroy(result);
}

static void test_invalid_json(void)
{
    JSON *json;
    Octstr *invalid;

    /* Missing closing brace */
    invalid = octstr_create("{\"name\":\"test\"");
    json = json_parse(invalid);
    if (json != NULL) {
        json_destroy(json);
        panic(0, "check_json: should reject unclosed object");
    }
    octstr_destroy(invalid);

    /* Invalid value */
    invalid = octstr_create("{\"name\":undefined}");
    json = json_parse(invalid);
    if (json != NULL) {
        json_destroy(json);
        panic(0, "check_json: should reject undefined value");
    }
    octstr_destroy(invalid);

    /* Empty input */
    invalid = octstr_create("");
    json = json_parse(invalid);
    if (json != NULL) {
        json_destroy(json);
        panic(0, "check_json: should reject empty input");
    }
    octstr_destroy(invalid);
}

int main(void)
{
    gwlib_init();
    log_set_output_level(GW_INFO);

    test_parse_object();
    test_parse_array();
    test_parse_nested();
    test_parse_escapes();
    test_generate_object();
    test_escape_string();
    test_response();
    test_invalid_json();

    gwlib_shutdown();
    return 0;
}
