

#ifndef OBJECT_H
#define OBJECT_H

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <math.h>

typedef void (*Method)();
extern const void * Object(void); 
extern const void * Class(void); 

//  selectors
void * new(const void * class, ...);
void delete(void * self);
void * convert_to(const void * class, ...);
void * ctor(void * self, va_list * app);
void * dtor(void * self);
int puto(void * self, FILE * fp);
void * forward(const void * self, Method selector, const char * tag, ...);
void * link(const void * class, void * self);

// static methods;
void * cast(const void * class, const void * self);
void doc(const void * class, const void * member);
const void * type(const void * self);
const size_t sizeOf(const void * self);
const size_t num_methods(const void * class);

Method super(const void * class, const void * method);

const void * mro(void * self, int i);
void * base(void * self, int i);
bool is_object(const void * self);
bool isinstance(const void * self, const void * class);
bool issubclass(const void * class, const void * superclass);

#define object_t            Object()
#define type_t              Class()

#endif
