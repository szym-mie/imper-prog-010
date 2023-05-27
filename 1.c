#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX_STR_LEN 64

typedef unsigned char byte;

typedef struct Vector {
	void *data;
	size_t element_size;
	size_t size;
	size_t capacity;
} Vector;

typedef struct Person {
	int age;
	char first_name[MAX_STR_LEN];
	char last_name[MAX_STR_LEN];
} Person;

typedef int(*cmp_ptr)(const void*, const void*);
typedef int(*predicate_ptr)(void*);
typedef void(*read_ptr)(void*);
typedef void(*print_ptr)(const void*);

#define VECTOR_OFFSET(V, S) ((V).element_size * (S))

#define VECTOR_INIT_ZERO // comment to disable zero-init at 'resize'
#define VECTOR_ALLOC_MARGIN 0 // how much slots to add when resizing
#define VECTOR_ALLOC_THRES 0 // alloc threshold upsizes with N free slots

// Allocate vector to initial capacity (block_size elements),
// Set element_size, size (to 0), capacity
void 
init_vector(Vector *vector, size_t block_size, size_t element_size)
{
	vector->element_size = element_size;
	vector->data = malloc(VECTOR_OFFSET(*vector, block_size));
	if (vector->data == NULL) return; // fail silently
	vector->size = 0;
	vector->capacity = block_size;
}

// If new_capacity is greater than the current capacity,
// new storage is allocated, otherwise the function does nothing.
void 
reserve(Vector *vector, size_t new_capacity) 
{
	if (new_capacity > vector->capacity)
	{
		void *new_data = realloc(vector->data, VECTOR_OFFSET(*vector, new_capacity));
		if (new_data == NULL) return; // fail silently
		void *new_data_offset = (byte *)new_data + VECTOR_OFFSET(*vector, vector->capacity);
		vector->data = new_data;
		vector->capacity = new_capacity;
	}
}

// Resizes the vector to contain new_size elements.
// If the current size is greater than new_size, the container is
// reduced to its first new_size elements.

// If the current size is less than new_size,
// additional zero-initialized elements are appended
void 
resize(Vector *vector, size_t new_size) {
	size_t new_capacity = vector->capacity * 2 + VECTOR_ALLOC_MARGIN;
	// in case 'new_size' greater than 'new_capacity'
	// the biggest danger here is silent fail to allocate further memory
	// leading to segfaults
	while (new_size + VECTOR_ALLOC_THRES > vector->capacity) 
		reserve(vector, new_capacity);

	void *prev_end = (byte *)vector->data + VECTOR_OFFSET(*vector, vector->size);
#ifdef VECTOR_INIT_ZERO
	// clear new elements to zero
	memset(prev_end, 0, VECTOR_OFFSET(*vector, vector->capacity - vector->size));
#endif

	vector->size = new_size;
}

// Add element to the end of the vector
void 
push_back(Vector *vector, void *value) 
{
	// GCC extensions allow for direct pointer arithemtic on 'void *' by assuming
	// sizeof(void *) to be equal to 1. Use casting to stay compatible with ANSI C
	size_t prev_size = vector->size;
	resize(vector, vector->size + 1);
	void *ins_point = (byte *)vector->data + VECTOR_OFFSET(*vector, prev_size);
	memcpy(ins_point, value, vector->element_size);
}

// Remove all elements from the vector
void 
clear(Vector *vector)
{
	free(vector->data);
	vector->capacity = 0;
	vector->size = 0;
}

// Insert new element at index (0 <= index <= size) position
void 
insert(Vector *vector, size_t index, void *value)
{
	resize(vector, vector->size + 1);
	size_t prev_size = vector->size;
	void *ins_point = (byte *)vector->data + VECTOR_OFFSET(*vector, index);
	void *move_point = (byte *)ins_point + vector->element_size;
	size_t cpy_size = VECTOR_OFFSET(*vector, prev_size - index - 1);
	memmove(move_point, ins_point, cpy_size);
	memcpy(ins_point, value, vector->element_size);
}

// Erase element at position index
void 
erase(Vector *vector, size_t index) 
{
	size_t prev_size = vector->size;
	void *move_point = (byte *)vector->data + VECTOR_OFFSET(*vector, index);
	void *del_point = (byte *)move_point + vector->element_size;
	size_t cpy_size = VECTOR_OFFSET(*vector, prev_size - index - 1);
	memmove(move_point, del_point, cpy_size);
	vector->size--;
}

#define VECTOR_FOREACH(V) for (size_t i = 0, pi = 0; i < (V).size; i++, pi+=(V).element_size)
#define VECTOR_STEPBACK(V) i--, pi-=(V).element_size;

// Erase all elements that compare equal to value from the container
void 
erase_value(Vector *vector, void *value, cmp_ptr cmp) 
{
	byte *blk = vector->data;
	VECTOR_FOREACH(*vector) 
	{
		if (!cmp(value, blk+pi)) 
		{
			erase(vector, i);
			VECTOR_STEPBACK(*vector)
		}
	}
}

// Erase all elements that satisfy the predicate from the vector
void 
erase_if(Vector *vector, int (*predicate)(void *)) 
{
	byte *blk = vector->data;
	VECTOR_FOREACH(*vector)
	{
		if (predicate(blk+pi)) 
		{
			erase(vector, i);
			VECTOR_STEPBACK(*vector)
		}
	}
}

// Request the removal of unused capacity
void 
shrink_to_fit(Vector *vector) 
{
	void *new_data = realloc(vector->data, VECTOR_OFFSET(*vector, vector->size));
	if (new_data == NULL) return;
	vector->data = new_data;
	vector->capacity = vector->size;
}

// integer comparator
int 
int_cmp(const void *v1, const void *v2) 
{
	const int *pv1 = v1;
	const int *pv2 = v2;
	return *pv1 - *pv2;
}

// char comparator
int 
char_cmp(const void *v1, const void *v2) 
{	
	const char *pv1 = v1;
	const char *pv2 = v2;
	return *pv1 - *pv2;
}

// Person comparator:
// Sort according to age (decreasing)
// When ages equal compare first name and then last name
int 
person_cmp(const void *p1, const void *p2) 
{
	const Person *pp1 = p1;
	const Person *pp2 = p2;
	int age_diff = pp2->age - pp1->age;
	if (age_diff) return age_diff;
	int first_name_diff = strncmp(pp1->first_name, pp2->first_name, MAX_STR_LEN);
	if (first_name_diff) return first_name_diff;
	return strncmp(pp1->last_name, pp2->last_name, MAX_STR_LEN);
}

// predicate: check if number is even
int 
is_even(void *value) 
{
	int *iv = value;
	return *iv % 2 == 0;
}

// predicate: check if char is a vowel
int 
is_vowel(void *value) 
{
	char c = *((char *)value);
	return 
		c == 'a' || c == 'A' || 
		c == 'e' || c == 'E' || 
		c == 'i' || c == 'I' ||
		c == 'o' || c == 'O' ||
		c == 'u' || c == 'U' ||
		c == 'y' || c == 'Y';
}

// predicate: check if person is older than 25
int 
is_older_than_25(void *person) 
{
	Person *p = person;
	return p->age > 25;
}

// print integer value
void 
print_int(const void *v) 
{
	const int *iv = v;
	printf("%d ", *iv);
}

// print char value
void
print_char(const void *v) 
{
	const char *cv = v;
	printf("%c ", *cv);
}

// print structure Person
void 
print_person(const void *v) 
{
	const Person *pv = v;
	printf("%d %s %s\n", pv->age, pv->first_name, pv->last_name);
}

// print capacity of the vector and its elements
void 
print_vector(Vector *vector, print_ptr print) 
{
	printf("%zu\n", vector->capacity);

	byte *blk = vector->data;
	VECTOR_FOREACH(*vector)
	{
		print(blk+pi);
	}
}

// read int value
void 
read_int(void* value) 
{
	scanf("%d", (int *)value);
}

// read char value
void 
read_char(void* value) 
{
	scanf(" %c", (char *)value);
}

// read struct Person
void 
read_person(void* value) 
{
	Person *p = value;
	scanf("%d %s %s", &p->age, p->first_name, p->last_name);
}

void vector_test(Vector *vector, size_t block_size, size_t elem_size, int n, read_ptr read,
		 cmp_ptr cmp, predicate_ptr predicate, print_ptr print) {
	init_vector(vector, block_size, elem_size);
	void *v = malloc(vector->element_size);
	if (v == NULL) return; // the safe part of safe_malloc
	size_t index, size;
	for (int i = 0; i < n; ++i) {
		char op;
		scanf(" %c", &op);
		switch (op) {
			case 'p': // push_back
				read(v);
				push_back(vector, v);
				break;
			case 'i': // insert
				scanf("%zu", &index);
				read(v);
				insert(vector, index, v);
				break;
			case 'e': // erase
				scanf("%zu", &index);
				erase(vector, index);
				break;
			case 'v': // erase
				read(v);
				erase_value(vector, v, cmp);
				break;
			case 'd': // erase (predicate)
				erase_if(vector, predicate);
				break;
			case 'r': // resize
				scanf("%zu", &size);
				resize(vector, size);
				break;
			case 'c': // clear
				clear(vector);
				break;
			case 'f': // shrink
				shrink_to_fit(vector);
				break;
			case 's': // sort
				qsort(vector->data, vector->size,
				      vector->element_size, cmp);
				break;
			default:
				printf("No such operation: %c\n", op);
				break;
		}
	}
	print_vector(vector, print);
	free(vector->data);
	free(v);
}

int main(void) {
	int to_do, n;
	Vector vector_int, vector_char, vector_person;

	scanf("%d%d", &to_do, &n);

	switch (to_do) {
		case 1:
			vector_test(&vector_int, 4, sizeof(int), n, read_int, int_cmp,
				is_even, print_int);
			break;
		case 2:
			vector_test(&vector_char, 2, sizeof(char), n, read_char, char_cmp,
				is_vowel, print_char);
			break;
		case 3:
			vector_test(&vector_person, 2, sizeof(Person), n, read_person,
				person_cmp, is_older_than_25, print_person);
			break;
		default:
			printf("Nothing to do for %d\n", to_do);
			break;
	}

	return 0;
}
