/*
 * json.c - Lightweight JSON parsing and generation for Kannel
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

#include "gwlib.h"
#include "json.h"

/* Maximum nesting depth to prevent stack overflow */
#define JSON_NESTING_LIMIT 1000

/* Internal JSON structure (based on cJSON) */
struct JSON {
    struct JSON *next;
    struct JSON *prev;
    struct JSON *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;  /* key name if this is an object member */
};

/* Parse buffer */
typedef struct {
    const unsigned char *content;
    size_t length;
    size_t offset;
    size_t depth;
} parse_buffer;

/* Buffer access macros */
#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
#define can_access_at_index(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)

/* Forward declarations */
static int parse_value(JSON *item, parse_buffer *input_buffer);
static int parse_string(JSON *item, parse_buffer *input_buffer);
static int parse_number(JSON *item, parse_buffer *input_buffer);
static int parse_array(JSON *item, parse_buffer *input_buffer);
static int parse_object(JSON *item, parse_buffer *input_buffer);

/*
 * Create a new JSON node
 */
static JSON *json_new_item(void)
{
    JSON *node = gw_malloc(sizeof(JSON));
    memset(node, 0, sizeof(JSON));
    return node;
}

/*
 * Skip whitespace
 */
static parse_buffer *buffer_skip_whitespace(parse_buffer *buffer)
{
    if (buffer == NULL || buffer->content == NULL)
        return NULL;

    while (can_access_at_index(buffer, 0) && buffer_at_offset(buffer)[0] <= 32)
        buffer->offset++;

    return buffer;
}

/*
 * Skip UTF-8 BOM if present
 */
static parse_buffer *skip_utf8_bom(parse_buffer *buffer)
{
    if (buffer == NULL || buffer->content == NULL || buffer->offset != 0)
        return NULL;

    if (can_access_at_index(buffer, 2) &&
        buffer->content[0] == 0xEF &&
        buffer->content[1] == 0xBB &&
        buffer->content[2] == 0xBF) {
        buffer->offset += 3;
    }

    return buffer;
}

/*
 * Parse 4 hex digits for unicode escape
 */
static unsigned int parse_hex4(const unsigned char *str)
{
    unsigned int h = 0;
    int i;

    for (i = 0; i < 4; i++) {
        unsigned char c = str[i];
        if (c >= '0' && c <= '9')
            h = (h << 4) + (c - '0');
        else if (c >= 'A' && c <= 'F')
            h = (h << 4) + (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f')
            h = (h << 4) + (c - 'a' + 10);
        else
            return 0;
    }
    return h;
}

/*
 * Convert UTF-16 escape to UTF-8
 */
static unsigned char utf16_to_utf8(const unsigned char *input_pointer,
                                   const unsigned char *input_end,
                                   unsigned char **output_pointer)
{
    unsigned long codepoint = 0;
    unsigned int first_code = 0;
    const unsigned char *first_sequence = input_pointer;
    unsigned char utf8_length = 0;
    unsigned char utf8_position = 0;
    unsigned char sequence_length = 0;
    unsigned char first_byte_mark = 0;

    if ((input_end - first_sequence) < 6)
        return 0;

    first_code = parse_hex4(first_sequence + 2);
    if ((first_code >= 0xDC00) && (first_code <= 0xDFFF))
        return 0;

    /* UTF-16 surrogate pair */
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF)) {
        const unsigned char *second_sequence = first_sequence + 6;
        unsigned int second_code = 0;
        sequence_length = 12;

        if ((input_end - second_sequence) < 6)
            return 0;
        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u'))
            return 0;

        second_code = parse_hex4(second_sequence + 2);
        if ((second_code < 0xDC00) || (second_code > 0xDFFF))
            return 0;

        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    } else {
        sequence_length = 6;
        codepoint = first_code;
    }

    /* Encode as UTF-8 */
    if (codepoint < 0x80) {
        utf8_length = 1;
    } else if (codepoint < 0x800) {
        utf8_length = 2;
        first_byte_mark = 0xC0;
    } else if (codepoint < 0x10000) {
        utf8_length = 3;
        first_byte_mark = 0xE0;
    } else if (codepoint <= 0x10FFFF) {
        utf8_length = 4;
        first_byte_mark = 0xF0;
    } else {
        return 0;
    }

    for (utf8_position = (unsigned char)(utf8_length - 1); utf8_position > 0; utf8_position--) {
        (*output_pointer)[utf8_position] = (unsigned char)((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }

    if (utf8_length > 1)
        (*output_pointer)[0] = (unsigned char)((codepoint | first_byte_mark) & 0xFF);
    else
        (*output_pointer)[0] = (unsigned char)(codepoint & 0x7F);

    *output_pointer += utf8_length;
    return sequence_length;
}

/*
 * Parse a JSON string
 */
static int parse_string(JSON *item, parse_buffer *input_buffer)
{
    const unsigned char *input_pointer = buffer_at_offset(input_buffer) + 1;
    const unsigned char *input_end = buffer_at_offset(input_buffer) + 1;
    unsigned char *output_pointer = NULL;
    unsigned char *output = NULL;
    size_t allocation_length = 0;
    size_t skipped_bytes = 0;

    if (buffer_at_offset(input_buffer)[0] != '\"')
        return 0;

    /* Calculate output size */
    while (((size_t)(input_end - input_buffer->content) < input_buffer->length) && (*input_end != '\"')) {
        if (input_end[0] == '\\') {
            if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length)
                return 0;
            skipped_bytes++;
            input_end++;
        }
        input_end++;
    }

    if (((size_t)(input_end - input_buffer->content) >= input_buffer->length) || (*input_end != '\"'))
        return 0;

    allocation_length = (size_t)(input_end - buffer_at_offset(input_buffer)) - skipped_bytes;
    output = gw_malloc(allocation_length + 1);

    output_pointer = output;

    /* Copy and unescape */
    while (input_pointer < input_end) {
        if (*input_pointer != '\\') {
            *output_pointer++ = *input_pointer++;
        } else {
            unsigned char sequence_length = 2;
            if ((input_end - input_pointer) < 1)
                goto fail;

            switch (input_pointer[1]) {
                case 'b': *output_pointer++ = '\b'; break;
                case 'f': *output_pointer++ = '\f'; break;
                case 'n': *output_pointer++ = '\n'; break;
                case 'r': *output_pointer++ = '\r'; break;
                case 't': *output_pointer++ = '\t'; break;
                case '\"':
                case '\\':
                case '/':
                    *output_pointer++ = input_pointer[1];
                    break;
                case 'u':
                    sequence_length = utf16_to_utf8(input_pointer, input_end, &output_pointer);
                    if (sequence_length == 0)
                        goto fail;
                    break;
                default:
                    goto fail;
            }
            input_pointer += sequence_length;
        }
    }

    *output_pointer = '\0';
    item->type = JSON_STRING;
    item->valuestring = (char *)output;
    input_buffer->offset = (size_t)(input_end - input_buffer->content);
    input_buffer->offset++;

    return 1;

fail:
    gw_free(output);
    return 0;
}

/*
 * Parse a JSON number
 */
static int parse_number(JSON *item, parse_buffer *input_buffer)
{
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char *number_c_string = NULL;
    size_t i = 0;
    size_t number_string_length = 0;

    if (input_buffer == NULL || input_buffer->content == NULL)
        return 0;

    /* Find length of number string */
    for (i = 0; can_access_at_index(input_buffer, i); i++) {
        unsigned char c = buffer_at_offset(input_buffer)[i];
        if ((c >= '0' && c <= '9') || c == '+' || c == '-' ||
            c == 'e' || c == 'E' || c == '.') {
            number_string_length++;
        } else {
            break;
        }
    }

    number_c_string = gw_malloc(number_string_length + 1);
    memcpy(number_c_string, buffer_at_offset(input_buffer), number_string_length);
    number_c_string[number_string_length] = '\0';

    number = strtod((const char *)number_c_string, (char **)&after_end);
    if (number_c_string == after_end) {
        gw_free(number_c_string);
        return 0;
    }

    item->valuedouble = number;
    if (number >= INT_MAX)
        item->valueint = INT_MAX;
    else if (number <= (double)INT_MIN)
        item->valueint = INT_MIN;
    else
        item->valueint = (int)number;

    item->type = JSON_NUMBER;
    input_buffer->offset += (size_t)(after_end - number_c_string);
    gw_free(number_c_string);

    return 1;
}

/*
 * Parse a JSON value
 */
static int parse_value(JSON *item, parse_buffer *input_buffer)
{
    if (input_buffer == NULL || input_buffer->content == NULL)
        return 0;

    /* null */
    if (can_read(input_buffer, 4) &&
        strncmp((const char *)buffer_at_offset(input_buffer), "null", 4) == 0) {
        item->type = JSON_NULL;
        input_buffer->offset += 4;
        return 1;
    }

    /* false */
    if (can_read(input_buffer, 5) &&
        strncmp((const char *)buffer_at_offset(input_buffer), "false", 5) == 0) {
        item->type = JSON_FALSE;
        input_buffer->offset += 5;
        return 1;
    }

    /* true */
    if (can_read(input_buffer, 4) &&
        strncmp((const char *)buffer_at_offset(input_buffer), "true", 4) == 0) {
        item->type = JSON_TRUE;
        item->valueint = 1;
        input_buffer->offset += 4;
        return 1;
    }

    /* string */
    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == '\"')
        return parse_string(item, input_buffer);

    /* number */
    if (can_access_at_index(input_buffer, 0) &&
        (buffer_at_offset(input_buffer)[0] == '-' ||
         (buffer_at_offset(input_buffer)[0] >= '0' && buffer_at_offset(input_buffer)[0] <= '9')))
        return parse_number(item, input_buffer);

    /* array */
    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == '[')
        return parse_array(item, input_buffer);

    /* object */
    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == '{')
        return parse_object(item, input_buffer);

    return 0;
}

/*
 * Parse a JSON array
 */
static int parse_array(JSON *item, parse_buffer *input_buffer)
{
    JSON *head = NULL;
    JSON *current_item = NULL;

    if (input_buffer->depth >= JSON_NESTING_LIMIT)
        return 0;
    input_buffer->depth++;

    if (buffer_at_offset(input_buffer)[0] != '[')
        goto fail;

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);

    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == ']')
        goto success;

    if (cannot_access_at_index(input_buffer, 0)) {
        input_buffer->offset--;
        goto fail;
    }

    input_buffer->offset--;

    do {
        JSON *new_item = json_new_item();

        if (head == NULL) {
            current_item = head = new_item;
        } else {
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);

        if (!parse_value(current_item, input_buffer))
            goto fail;

        buffer_skip_whitespace(input_buffer);
    } while (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == ',');

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ']')
        goto fail;

success:
    input_buffer->depth--;
    if (head != NULL)
        head->prev = current_item;

    item->type = JSON_ARRAY;
    item->child = head;
    input_buffer->offset++;

    return 1;

fail:
    if (head != NULL)
        json_destroy(head);
    return 0;
}

/*
 * Parse a JSON object
 */
static int parse_object(JSON *item, parse_buffer *input_buffer)
{
    JSON *head = NULL;
    JSON *current_item = NULL;

    if (input_buffer->depth >= JSON_NESTING_LIMIT)
        return 0;
    input_buffer->depth++;

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != '{')
        goto fail;

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);

    if (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == '}')
        goto success;

    if (cannot_access_at_index(input_buffer, 0)) {
        input_buffer->offset--;
        goto fail;
    }

    input_buffer->offset--;

    do {
        JSON *new_item = json_new_item();

        if (head == NULL) {
            current_item = head = new_item;
        } else {
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        if (cannot_access_at_index(input_buffer, 1))
            goto fail;

        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);

        if (!parse_string(current_item, input_buffer))
            goto fail;

        buffer_skip_whitespace(input_buffer);

        /* Swap valuestring and string (we parsed the key name) */
        current_item->string = current_item->valuestring;
        current_item->valuestring = NULL;

        if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ':')
            goto fail;

        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);

        if (!parse_value(current_item, input_buffer))
            goto fail;

        buffer_skip_whitespace(input_buffer);
    } while (can_access_at_index(input_buffer, 0) && buffer_at_offset(input_buffer)[0] == ',');

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != '}')
        goto fail;

success:
    input_buffer->depth--;
    if (head != NULL)
        head->prev = current_item;

    item->type = JSON_OBJECT;
    item->child = head;
    input_buffer->offset++;

    return 1;

fail:
    if (head != NULL)
        json_destroy(head);
    return 0;
}

/*
 * Public API: Parse JSON from Octstr
 */
JSON *json_parse(Octstr *json)
{
    if (json == NULL)
        return NULL;
    return json_parse_cstr(octstr_get_cstr(json), octstr_len(json));
}

/*
 * Public API: Parse JSON from C string
 */
JSON *json_parse_cstr(const char *json, long len)
{
    parse_buffer buffer = {0, 0, 0, 0};
    JSON *item = NULL;

    if (json == NULL || len == 0)
        return NULL;

    buffer.content = (const unsigned char *)json;
    buffer.length = len;
    buffer.offset = 0;
    buffer.depth = 0;

    item = json_new_item();

    if (!parse_value(item, buffer_skip_whitespace(skip_utf8_bom(&buffer)))) {
        json_destroy(item);
        return NULL;
    }

    return item;
}

/*
 * Public API: Destroy JSON and free memory
 */
void json_destroy(JSON *item)
{
    JSON *next = NULL;

    while (item != NULL) {
        next = item->next;

        if (item->child != NULL)
            json_destroy(item->child);

        if (item->valuestring != NULL)
            gw_free(item->valuestring);

        if (item->string != NULL)
            gw_free(item->string);

        gw_free(item);
        item = next;
    }
}

/*
 * Public API: Get JSON type
 */
int json_type(const JSON *json)
{
    if (json == NULL)
        return JSON_INVALID;
    return json->type;
}

int json_is_object(const JSON *json)
{
    return json != NULL && json->type == JSON_OBJECT;
}

int json_is_array(const JSON *json)
{
    return json != NULL && json->type == JSON_ARRAY;
}

int json_is_string(const JSON *json)
{
    return json != NULL && json->type == JSON_STRING;
}

int json_is_number(const JSON *json)
{
    return json != NULL && json->type == JSON_NUMBER;
}

int json_is_bool(const JSON *json)
{
    return json != NULL && (json->type == JSON_TRUE || json->type == JSON_FALSE);
}

int json_is_null(const JSON *json)
{
    return json != NULL && json->type == JSON_NULL;
}

/*
 * Public API: Get field from object (case-insensitive)
 */
JSON *json_get(const JSON *json, const char *key)
{
    JSON *current;

    if (json == NULL || key == NULL || json->type != JSON_OBJECT)
        return NULL;

    current = json->child;
    while (current != NULL) {
        if (current->string != NULL && strcasecmp(key, current->string) == 0)
            return current;
        current = current->next;
    }

    return NULL;
}

/*
 * Public API: Get string value from object
 */
Octstr *json_get_string(const JSON *json, const char *key)
{
    JSON *item = json_get(json, key);
    if (item == NULL || item->type != JSON_STRING || item->valuestring == NULL)
        return NULL;
    return octstr_create(item->valuestring);
}

/*
 * Public API: Get integer value from object
 */
long json_get_integer(const JSON *json, const char *key, long default_val)
{
    JSON *item = json_get(json, key);
    if (item == NULL || item->type != JSON_NUMBER)
        return default_val;
    return item->valueint;
}

/*
 * Public API: Get double value from object
 */
double json_get_double(const JSON *json, const char *key, double default_val)
{
    JSON *item = json_get(json, key);
    if (item == NULL || item->type != JSON_NUMBER)
        return default_val;
    return item->valuedouble;
}

/*
 * Public API: Get boolean value from object
 */
int json_get_bool(const JSON *json, const char *key)
{
    JSON *item = json_get(json, key);
    if (item == NULL)
        return -1;
    if (item->type == JSON_TRUE)
        return 1;
    if (item->type == JSON_FALSE)
        return 0;
    return -1;
}

/*
 * Public API: Get string value from JSON node
 */
Octstr *json_value_string(const JSON *json)
{
    if (json == NULL || json->type != JSON_STRING || json->valuestring == NULL)
        return NULL;
    return octstr_create(json->valuestring);
}

/*
 * Public API: Get integer value from JSON node
 */
long json_value_integer(const JSON *json)
{
    if (json == NULL || json->type != JSON_NUMBER)
        return 0;
    return json->valueint;
}

/*
 * Public API: Get double value from JSON node
 */
double json_value_double(const JSON *json)
{
    if (json == NULL || json->type != JSON_NUMBER)
        return 0.0;
    return json->valuedouble;
}

/*
 * Public API: Get boolean value from JSON node
 */
int json_value_bool(const JSON *json)
{
    if (json == NULL)
        return 0;
    if (json->type == JSON_TRUE)
        return 1;
    return 0;
}

/*
 * Public API: Get array size
 */
int json_array_size(const JSON *json)
{
    JSON *child;
    int size = 0;

    if (json == NULL || json->type != JSON_ARRAY)
        return 0;

    child = json->child;
    while (child != NULL) {
        size++;
        child = child->next;
    }

    return size;
}

/*
 * Public API: Get array item by index
 */
JSON *json_array_get(const JSON *json, int index)
{
    JSON *current;

    if (json == NULL || json->type != JSON_ARRAY || index < 0)
        return NULL;

    current = json->child;
    while (current != NULL && index > 0) {
        index--;
        current = current->next;
    }

    return current;
}

/*
 * Public API: Escape string for JSON output
 */
Octstr *json_escape_string(Octstr *str)
{
    Octstr *result;
    long i, len;

    if (str == NULL)
        return octstr_create("");

    result = octstr_create("");
    len = octstr_len(str);

    for (i = 0; i < len; i++) {
        int c = octstr_get_char(str, i);
        switch (c) {
            case '\"': octstr_append_cstr(result, "\\\""); break;
            case '\\': octstr_append_cstr(result, "\\\\"); break;
            case '\b': octstr_append_cstr(result, "\\b"); break;
            case '\f': octstr_append_cstr(result, "\\f"); break;
            case '\n': octstr_append_cstr(result, "\\n"); break;
            case '\r': octstr_append_cstr(result, "\\r"); break;
            case '\t': octstr_append_cstr(result, "\\t"); break;
            default:
                if (c < 32) {
                    octstr_format_append(result, "\\u%04x", c);
                } else {
                    octstr_append_char(result, c);
                }
                break;
        }
    }

    return result;
}

/*
 * Public API: Create JSON object from format string
 * Format: s=Octstr*, S=char*, i=long, d=double, b=int(bool), n=null
 */
Octstr *json_object(const char *fmt, ...)
{
    Octstr *result;
    va_list ap;
    const char *p;
    int first = 1;

    result = octstr_create("{");
    va_start(ap, fmt);

    for (p = fmt; *p; p++) {
        const char *key = va_arg(ap, const char *);

        if (!first)
            octstr_append_char(result, ',');
        first = 0;

        octstr_format_append(result, "\"%s\":", key);

        switch (*p) {
            case 's': {
                /* Octstr* */
                Octstr *val = va_arg(ap, Octstr *);
                if (val != NULL) {
                    Octstr *escaped = json_escape_string(val);
                    octstr_format_append(result, "\"%S\"", escaped);
                    octstr_destroy(escaped);
                } else {
                    octstr_append_cstr(result, "null");
                }
                break;
            }
            case 'S': {
                /* char* */
                const char *val = va_arg(ap, const char *);
                if (val != NULL) {
                    Octstr *tmp = octstr_create(val);
                    Octstr *escaped = json_escape_string(tmp);
                    octstr_format_append(result, "\"%S\"", escaped);
                    octstr_destroy(escaped);
                    octstr_destroy(tmp);
                } else {
                    octstr_append_cstr(result, "null");
                }
                break;
            }
            case 'i': {
                /* long */
                long val = va_arg(ap, long);
                octstr_format_append(result, "%ld", val);
                break;
            }
            case 'd': {
                /* double */
                double val = va_arg(ap, double);
                octstr_format_append(result, "%g", val);
                break;
            }
            case 'b': {
                /* bool (int) */
                int val = va_arg(ap, int);
                octstr_append_cstr(result, val ? "true" : "false");
                break;
            }
            case 'n': {
                /* null (consume dummy arg for consistency) */
                (void)va_arg(ap, void *);
                octstr_append_cstr(result, "null");
                break;
            }
            default:
                /* Unknown format, skip */
                (void)va_arg(ap, void *);
                octstr_append_cstr(result, "null");
                break;
        }
    }

    va_end(ap);
    octstr_append_char(result, '}');

    return result;
}

/*
 * Public API: Create simple response object
 */
Octstr *json_response(const char *key1, long val1, const char *key2, const char *val2)
{
    Octstr *result = octstr_create("{");

    octstr_format_append(result, "\"%s\":%ld", key1, val1);

    if (key2 != NULL && val2 != NULL) {
        Octstr *tmp = octstr_create(val2);
        Octstr *escaped = json_escape_string(tmp);
        octstr_format_append(result, ",\"%s\":\"%S\"", key2, escaped);
        octstr_destroy(escaped);
        octstr_destroy(tmp);
    }

    octstr_append_char(result, '}');
    return result;
}
