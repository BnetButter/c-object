#ifndef _OBJECT_H
#define _OBJECT_H

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#define MAGIC 0x0effaced

struct super
{
    int count; 
    const size_t * offset; // offset of base classes
    const struct class ** super;
};

struct mro
{
    int count;
    const struct class ** mro;
};

struct method
{
    const char * tag; // method name
    void (*selector)();
    void (*method)();
    const char * docstring;
};



struct object 
{
    const void * class;
};

struct class
{
    const struct object _;
    const char * name; // class name
    const struct super super; 
    const struct mro mro;
    size_t size;
    const char * doc;
    long magic; // identifier


    struct method ctor;
    struct method dtor;
    struct method puto;
    struct method forward;
    struct method new;
    struct method delete;
    struct method convert_to;
};



#endif
