#pragma once

#include "genc.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum JsonTag {
    JSON_NUMBER = 0,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL = 0xF
};

struct JsonNode;

#define JSON_VALUE_PAYLOAD_MASK 0x00007FFFFFFFFFFFULL
#define JSON_VALUE_NAN_MASK 0x7FF8000000000000ULL
#define JSON_VALUE_TAG_MASK 0xF
#define JSON_VALUE_TAG_SHIFT 47

union JsonValue {
    uint64_t ival;
    double fval;

    JsonValue(double x)
        : fval(x) {
    }
    JsonValue(JsonTag tag = JSON_NULL, void *payload = nullptr) {
        assert((uintptr_t)payload <= JSON_VALUE_PAYLOAD_MASK);
        ival = JSON_VALUE_NAN_MASK | ((uint64_t)tag << JSON_VALUE_TAG_SHIFT) | (uintptr_t)payload;
    }
    bool isDouble() const {
        return (int64_t)ival <= (int64_t)JSON_VALUE_NAN_MASK;
    }
    JsonTag getTag() const {
        return isDouble() ? JSON_NUMBER : JsonTag((ival >> JSON_VALUE_TAG_SHIFT) & JSON_VALUE_TAG_MASK);
    }
    uint64_t getPayload() const {
        assert(!isDouble());
        return ival & JSON_VALUE_PAYLOAD_MASK;
    }
    double toNumber() const {
        assert(getTag() == JSON_NUMBER);
        return fval;
    }
    const char *toString() const {
        assert(getTag() == JSON_STRING);
        return (const char *)getPayload();
    }
    JsonNode *toNode() const {
        assert(getTag() == JSON_ARRAY || getTag() == JSON_OBJECT);
        return (JsonNode *)getPayload();
    }
};


struct JsonNode {
    JsonValue value;
    JsonNode *next;
    char *key;
};

struct JsonIterator {
    JsonNode *p;

    void operator++() {
        p = p->next;
    }
    bool operator!=(const JsonIterator &x) const {
        return p != x.p;
    }
    JsonNode *operator*() const {
        return p;
    }
    JsonNode *operator->() const {
        return p;
    }
};

inline JsonIterator begin(JsonValue o) {
    return JsonIterator{o.toNode()};
}
inline JsonIterator end(JsonValue) {
    return JsonIterator{nullptr};
}

#define JSON_ERRNO_MAP(XX)                           \
    XX(OK, "ok")                                     \
    XX(BAD_NUMBER, "bad number")                     \
    XX(BAD_STRING, "bad string")                     \
    XX(BAD_IDENTIFIER, "bad identifier")             \
    XX(STACK_OVERFLOW, "stack overflow")             \
    XX(STACK_UNDERFLOW, "stack underflow")           \
    XX(MISMATCH_BRACKET, "mismatch bracket")         \
    XX(UNEXPECTED_CHARACTER, "unexpected character") \
    XX(UNQUOTED_KEY, "unquoted key")                 \
    XX(BREAKING_BAD, "breaking bad")                 \
    XX(ALLOCATION_FAILURE, "allocation failure")

enum JsonErrno {
#define XX(no, str) JSON_##no,
    JSON_ERRNO_MAP(XX)
#undef XX
};

const char *jsonStrError(int err);

#define JSON_ZONE_SIZE 4096
#define JSON_STACK_SIZE 32

class JsonAllocator {
    struct Zone {
        Zone *next;
        size_t used;
    } *head;

public:
    JsonAllocator() : head(nullptr) {};
    JsonAllocator(const JsonAllocator &) = delete;
    JsonAllocator &operator=(const JsonAllocator &) = delete;
    JsonAllocator(JsonAllocator &&x) : head(x.head) {
        x.head = nullptr;
    }
    JsonAllocator &operator=(JsonAllocator &&x) {
        head = x.head;
        x.head = nullptr;
        return *this;
    }
    ~JsonAllocator() {
        deallocate();
    }
	void *allocate(size_t size) {
		size = (size + 7) & ~7;

		if (head && head->used + size <= JSON_ZONE_SIZE) {
			char *p = (char *)head + head->used;
			head->used += size;
			return p;
		}

		size_t allocSize = sizeof(Zone) + size;
		Zone *zone = (Zone *)malloc(allocSize <= JSON_ZONE_SIZE ? JSON_ZONE_SIZE : allocSize);
		if (zone == nullptr)
			return nullptr;
		zone->used = allocSize;
		if (allocSize <= JSON_ZONE_SIZE || head == nullptr) {
			zone->next = head;
			head = zone;
		}
		else {
			zone->next = head->next;
			head->next = zone;
		}
		return (char *)zone + sizeof(Zone);
	}

	void deallocate() {
		while (head) {
			Zone *next = head->next;
			free(head);
			head = next;
		}
	}
};

namespace JsonParser {

	inline bool isspace(char c) {
		return c == ' ' || (c >= '\t' && c <= '\r');
	}

	inline bool isdelim(char c) {
		return c == ',' || c == ':' || c == ']' || c == '}' || isspace(c) || !c;
	}

	inline bool isdigit(char c) {
		return c >= '0' && c <= '9';
	}

	inline bool isxdigit(char c) {
		return (c >= '0' && c <= '9') || ((c & ~' ') >= 'A' && (c & ~' ') <= 'F');
	}

	inline int char2int(char c) {
		if (c <= '9')
			return c - '0';
		return (c & ~' ') - 'A' + 10;
	}

	double string2double(char *s, char **endptr);
	JsonNode *insertAfter(JsonNode *tail, JsonNode *node);
	JsonValue listToValue(JsonTag tag, JsonNode *tail);

} //namespace JsonParser

int jsonParse(char *,char **,JsonValue *,JsonAllocator&);
