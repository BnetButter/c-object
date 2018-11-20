#include "object.h"
#include "_object.h"



/* static methods */
const void * type(const void * _self)
{
    const struct object * self = _self;
    assert(self && self->class);
    return self->class;
}
static const char * _method_doc(const struct class * class, Method selector)
{   
	if (selector){
		const struct method * p = & class->ctor;
		int _num_methods = 
			(int) ((sizeOf(class)- offsetof(struct class, ctor))
			/ sizeof(struct method));
		do
			if (p->selector == selector)
				return p->docstring;
        while (++p, -- _num_methods);
	}
    static const char unavailable[] = {"method docstring unavailable"};
	return unavailable;
}
void doc(const void * _class, const void * element)
{
    const struct class * class = _class;
    if (element == NULL || element == class)
        printf("%s\n", class->doc);
    else {
        const char * docstring = _method_doc(class, (Method) element);
        printf("%s\n", docstring);
    }
}

const char * nameOf(const void * _self)
{
    const struct class * class = type(_self);
    assert(class->name && *class->name);
    return class->name;
}

const size_t sizeOf(const void * _self)
{
    const struct class * class = type(_self);
    assert(class && class->size);
    return class->size;
}

const size_t num_methods(const void * _self)
{
    const struct class * class = type(_self);
    return (sizeOf(class)- offsetof(struct class, ctor))
			/ sizeof(struct method);
    
}

Method super(const void * _class, const void * method)
{
    const struct class * class = _class;
    assert(method && class && class->magic == MAGIC);
    for (int i = 1; i < class->mro.count; i++) {
        const struct class * superclass = class->mro.mro[i];
        const struct method * p = & superclass->ctor;
        int num_methods = (int) ((sizeOf(superclass) - offsetof(struct class, ctor))
            / sizeof(struct method));
        do
            if (p->method && p->selector == method)
                return p->method;
        while (++p, -- num_methods);
    }
    fprintf(stderr, "Error: Method not found\n");
    exit(EXIT_FAILURE);
}



void * base(void * _self, int i)
{
    const struct class * class = type(_self);
    assert(i < class->super.count);
    return (void *)((char *) _self + class->super.offset[i]); 
}

bool is_object(const void * _self)
{   
    const struct class * class = type(_self);
    return class->magic == MAGIC;
}

void * allocate(const void * _class)
{
    const struct class * class = _class;
    struct object * result;
    assert(class && class->size);
    result = calloc(1, class->size);
    assert(result);
    return result;
}

bool isinstance(const void * _self, const void * _class)
{
    const struct class * class = type(_self);
    assert(class && class->magic);
    if (class->magic != MAGIC)
        return false;
    return _self && class == _class;
}

void * convert_to(const void * _class, ...)
{   
    const struct class * class = _class;
    assert(is_object(class));
    va_list ap; va_start(ap, _class);
    void * result;
    if (class->convert_to.method)
        result = ((void * (*)()) class->convert_to.method)(&ap);
    else
        result = forward(_class, (Method) convert_to, "convert_to", &ap);
    va_end(ap);
    return result;
}

const char * name(const void * _class)
{
    const struct class * class = _class;
    return class->name;
}
static void catch(int signum)
{
    if (signum == SIGSEGV)
        fprintf(stderr, "CastError: Uninitialized pointer. [SIGSEGV]\n"),
        exit(EXIT_FAILURE);
#ifdef SIGBUS
    if (signum == SIGBUS)
        fprintf(stderr, "CastError: Misaligned memory. [SIGBUS]\n"),
        exit(EXIT_FAILURE);
#endif
    if (signum)
        fprintf(stderr, "CastError: Exception found.\n"),
        exit(EXIT_FAILURE);
}

void * cast(const void * _class, const void * _self)
{
    void (*sigsegv)(int) = signal(SIGSEGV, catch);
#ifdef SIGBUS
    void (*sigbus)(int) = signal(SIGBUS, catch);
#endif
    {
        const struct object * self = _self;
        const struct class * typeOf_self = type(_self);
        assert("CastError: [self]'s type must a <class 'type'>\n" 
                && is_object(typeOf_self));
        
        if (_class != Object()) {
            assert("CastError: [class] is not of <class 'type'>\n" 
                    && is_object(_class));

            while (typeOf_self != _class) {
                assert("CastError: Illegal Cast.\n" 
                        && typeOf_self != Object());
                typeOf_self = typeOf_self->super.super[0];
            }
        }
    }
#ifdef SIGBUS
    signal(SIGBUS, sigbus);
#endif
    signal(SIGSEGV, sigsegv);
    return (void *) _self;
}


void * link(const void * _class, void * _self)
{
    const struct class * class = _class;
    struct object * self = _self;
    assert(class && self);
    self->class = class;
    return self;
}

/*
 * class selectors
 */
void * new(const void * _class, ...)
{
    const struct class * class = _class;
    assert(class && class->new.method);
    va_list ap; va_start(ap, _class);
    void * result = 
        ((void * (*)()) class->new.method)(class, &ap);
    va_end(ap);
    return result;
}

void delete(void * _self)
{   
    const struct class * class = type(_self);
    assert(class && class->delete.method);
    ((void (*)()) class->delete.method)(_self);
    
}

void * ctor(void * _self, va_list * app)
{
    const struct class * class = type(_self);
    assert(class->ctor.method);
    return ((void * (*)()) class->ctor.method)(_self, app);
}

void * dtor(void * _self)
{
    const struct class * class = type(_self);
    assert(class->ctor.method);
    return ((void * (*)()) class->dtor.method)(_self);
}

int puto(void * _self, FILE * fp)
{
    const struct class * class = type(_self);
    assert(class->puto.method);
    return ((int (*)()) class->puto.method)(_self, fp);
  
}

void * forward(const void * _self, Method selector, const char * tag, ...)
{
    const struct class * class = type(_self);
    assert(class->forward.method);
    va_list ap; va_start(ap, tag);
    void * result = ((void * (*)())class->forward.method)(_self, selector, tag, &ap);
    va_end(ap);
    return result;
}

/*
 * superclass selectors
*/

static void * object_ctor(void * _self, va_list * app)
{
    return _self;
}

static bool metaclass_conflict(void * _self)
{
    const struct class * class = type(_self);
    struct class * self = _self;
    
    bool conflict = false;
    for (int i = 0; i < self->super.count; i++) {
        const void * base_type = type(self->super.super[i]);
        if (! issubclass(class, base_type)) {
            conflict = true;
            break;
        }
    }
    return conflict;
}

static void create_super(void * _self, void * _super, va_list * app)
{   
    const struct class * class = type(_self);
    
    struct super * super = _super;
    size_t offset = offsetof(struct class, super);

    super->count = va_arg(*app, int);
    super->super = calloc(super->count, sizeof(void *));
    super->offset = calloc(super->count, sizeof(size_t));
    super->super[0] = va_arg(*app, const void *);

    size_t size = 0;
    for (int i = 1; i < super->count; i++) 
        super->super[i] = va_arg(*app, const void *);

    if (metaclass_conflict(_self)) {
        fprintf(stderr, "Error: Metaclass Conflict\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < super->count; i++) {
        (*(size_t *) (super->offset + i)) = size;
        size += (((const struct class *) (super->super[i]))->size);
    }
}

static void create_mro(void *, struct mro *);

static void * class_ctor(void * _self, va_list * app)
{
    struct class * self = _self;
    size_t offset = offsetof(struct class, ctor);
    self->name = va_arg(*app, const char *);
    create_super(self, (void *) &self->super, app);
    create_mro(self, (void *) &self->mro);
    self->size = va_arg(*app, const size_t);
    self->doc = va_arg(*app, const char *);
    self->magic = MAGIC;
    memcpy((char *) self + offset,
            (char *) self->super.super[0] + offset,
            sizeOf(self->super.super[0]) - offset);
    va_list ap; va_copy(ap, *app);
    Method selector;
    
	while ((selector = va_arg(ap, Method)))
	{	
		const char * tag = va_arg(ap, const char *);
		Method method = va_arg(ap, Method);
        const char * docstring = va_arg(ap, const char *);
		if (selector == (Method) ctor) {
            if (tag)
				self -> ctor.tag = tag,
				self -> ctor.selector = selector;
            if (docstring)
                self->ctor.docstring = docstring;
            self -> ctor.method = method;

			continue;
		}
		else if (selector == (Method) dtor) {
			if (tag)
				self -> dtor.tag = tag,
				self -> dtor.selector = selector;
            if (docstring)
                self->dtor.docstring = docstring;
            self -> dtor.method = method;

			continue;
		}
		else if (selector == (Method) puto) {
			if (tag)
				self -> puto.tag = tag,
				self -> puto.selector = selector;
            self->puto.method = method;
            if (docstring)
                self->puto.docstring = docstring;
			continue;
		}
		else if (selector == (Method) new) {
			if (tag)
				self -> new.tag = tag,
				self -> new.selector = selector;
            if (docstring)
                self->puto.docstring = docstring;
			self -> new.method = method;
			continue;
		}
		else if (selector == (Method) delete) {
			if (tag)
				self -> delete.tag = tag,
				self -> delete.selector = selector;
            if (docstring)
                self->delete.docstring = docstring;
			self -> delete.method = method;
			continue;
		}
		else if (selector == (Method) forward) {
			if (tag)
				self->forward.tag = tag,
				self->forward.selector = selector;
            if (docstring)
                self->forward.docstring = docstring;
			self->forward.method = method;
			continue;
		}
        else if (selector == (Method) convert_to) {
            if (tag)
                self->convert_to.tag = tag,
                self->convert_to.selector = selector;
            if (docstring)
                self->convert_to.docstring = docstring;
            self->convert_to.method = method;
            continue;
        }

	}
   
    va_end(ap);
	return self;
}

static void * object_dtor(void * _self)
{
    return _self;
}

static void * class_dtor(void * _self)
{
    fprintf(stderr, "Error: Cannot destroy types\n");
    exit(EXIT_FAILURE);
}

static int object_puto(void * _self, FILE * fp)
{   
    const struct class * class = type(_self);
    return fprintf(fp, "<class '%s'> at %p\n", class->name, _self);
}

static void * object_forward(void * _self, Method selector, const char * tag, va_list * app)
{   
    const struct class * class = type(_self);
    fprintf(stderr, "<class '%s'> at %p does not support '%s'\n", class->name, _self, tag);
}

static void * object_new(const void * _class, va_list * app)
{
    const struct class * class = _class;
    assert(class && class->super.super);
    struct object * result = allocate(_class);
    result -> class = _class;
    return ((void *(*)()) class->ctor.method)(result, app);
}

static void object_delete(void * _self)
{
    if(_self)
        free(dtor(_self));
}

/*
 * static class initialization  
*/
static const struct class _class;
static const struct class _object;

static const size_t offset_0[] = {(size_t) 0};
static const struct class * _object_supers[] = {NULL};
static const struct class * _object_linearization[] = {& _object};
static const struct class *  _class_linearization[] = {& _class, & _object};
static const char _object_docstring[] = {"*-docstring-*\n\nBase object. Supers all objects and types.\n*-end-*"};
static const char _class_docstring[] = {"**docstring**\nBase of all types. Creates and supers all types.\n*-end-*"};
static const struct class _object = {
    {& _class},
    "object",
    {0, offset_0, _object_supers}, 
    {1, _object_linearization},
    sizeof(struct object),
    _object_docstring,
    MAGIC,
    {"", (Method) ctor, (Method) object_ctor, NULL},
    {"", (Method) dtor, (Method) object_dtor, NULL},
    {"puto",(Method) puto,(Method) object_puto, NULL},
    {"forward", (Method) forward, (Method) object_forward, NULL},
    {"new", (Method) new, (Method) object_new, NULL},
    {"delete", (Method) delete, (Method) object_delete, NULL},
};

static const struct class _class = {
    {& _class},
    "type",
    {1, offset_0, _object_linearization},
    {2, _class_linearization},
    sizeof(struct class),
    _class_docstring,
     MAGIC,
    {"", (Method) ctor, (Method) class_ctor, NULL},
    {"", (Method) dtor, (Method) class_dtor, NULL},
    {"puto",(Method) puto, (Method) object_puto, NULL},
    {"forward", (Method) forward, (Method) object_forward, NULL},
    {"new", (Method) new, (Method) object_new, NULL},
    {"delete", (Method) delete, (Method) object_delete, NULL},
};


const void * Object(void)
{
    return & _object;
}

const void * Class(void)
{
    return & _class;
}

/* mro */

#define BASE_SIZE 8
#define COUNT(self) (((struct mro_util *)(self))->count)
struct mro_util
{   
    int count;
    int alloc_size;
    const void ** elements;
    const void * tag;
};

static void * mro_new(void * tag)
{
    struct mro_util * self = calloc(1, sizeof(struct mro_util));
    self->count = 0;
    self->alloc_size = BASE_SIZE;
    self->elements = calloc(self->alloc_size + 1, sizeof(void *));
    self->tag = tag;
    return self;
}

static void * mro_delete(void * _self)
{
    struct mro_util * self = _self;
    free(self->elements), self->elements = NULL;
    free(self), self = NULL;
    return self;
   
}

static void mro_append(void * _self, const void * element)
{
    struct mro_util * self = _self;
    if (self->count >= self->alloc_size) {
        self->elements = realloc(
                self->elements, 
                ((self->alloc_size * 2 * sizeof(void *))) + 1);
        self->alloc_size *= 2;
    }
    self->elements[self->count] = element;
    self->count ++;
}

static const void * mro_pop(void * _self, int index)
{
    struct mro_util * self = _self;
    if (self->count == 0)
        return NULL;

    const void * result = self->elements[index];
    for (; index < self->count; index++) {
        self->elements[index] = self->elements[index + 1];
    }
    self->count --;
    return result;
}

static int mro_index(void * _self, const void * element)
{
    struct mro_util * self = _self;
    for (int i = 0; i < self->count; i++) {
        if (self->elements[i] == element)
            return i;
    }
    fprintf(stderr, "IndexError: Item does not exist.\n");
    exit(1);
}



static const void * mro_get(void * _self, int index){
    struct mro_util * self = _self;
    if (self->count == 0)
        return NULL;
    else if (index >= self->count)
        return NULL;
    return self->elements[index];
}
static const void * mro_inner(void * _self, const void * element)
{
    struct mro_util * self = _self;
    for (int i = 0; i < self->count; i++) {
        const struct mro_util * e = self->elements[i];
        if (e->tag == element)
            return e;
    }
    return NULL;
}

static bool mro_contains(void * _self, const void * element)
{
    struct mro_util * self = _self;
    for (int i = 0; i < self->count; i++)
        if (self->elements[i] == element)
            return true;
    return false;
}

static const void * mro_remove_item(void * _self, const void * element)
{
    struct mro_util * self = _self;
    const void * result = NULL;
    for (int i = 0; i < self->count; i++)
        if (self->elements[i] == element) {
            result = self->elements[i];
            for (int j = i; j < self->count; j++)
                self->elements[j] = self->elements[j + 1];
            break;
        }
    self->count --;
    return result;
    
}

static void * add_to_graph(const void * _class, void * result)
{   
    const struct class * class = _class;
    if (! mro_contains(result, class)) {
        int i = 0;
        mro_append(result, _class);
        for (int i = 0; i < class->super.count; i++)
            add_to_graph(class->super.super[i], result);
    }
    return result;
}
static int compare_len(const void * _a, const void * _b)
{
    const struct class * const * a = _a;
    const struct class * const * b = _b;
    return (int) (*b)->super.count - (*a)->super.count;
}


static void * merge_sequence(void * _sequence, void * result)
{   
    struct mro_util * sequence = mro_new(NULL);
    for (int i = 0; i < COUNT(_sequence); i++) {
        const struct mro_util * subseq = mro_get(_sequence, i);
        
        void * subseq_copy = mro_new(NULL);
        for (int j = 0; j < COUNT(subseq); j++) 
            mro_append(subseq_copy, mro_get((void *) subseq, j));
        mro_append(sequence, subseq_copy);
    }

    const void * head;
    while (true) {
        // copy non-empty
        void * copy = mro_new(NULL);
        for (int i = 0; i < COUNT(sequence); i++) {
            const void * subseq = mro_get(sequence, i);
            if (COUNT(subseq))
                mro_append(copy, subseq);
        }
        if (! COUNT(copy)) {
            mro_delete(copy);
            for (int i = 0; i < COUNT(sequence); i++)
                mro_delete((void *) mro_get(sequence, i));
            mro_delete(sequence);
            return result;
        }

        bool is_clean = false;
        for (int i = 0; i < COUNT(copy); i++) {
            const void * subseq = mro_get(copy, i);
            head = mro_get((void *) subseq, 0);

            const void * sequence_slice = mro_new(NULL);
            
            for (int j = 0; j < COUNT(copy); j++)  {
                const void * sublist = mro_get(copy, j);
                void * sliced_sublist = mro_new(NULL);
                for (int k = 1; k < COUNT(sublist); k++)
                    mro_append(sliced_sublist, mro_get((void *)sublist, k));
                mro_append((void *)sequence_slice, sliced_sublist);
            }

            bool contains_head = false;
            for (int j = 0; j < COUNT(sequence_slice); j++) {
                const void * sublist_slice = mro_get((void *)sequence_slice, j);
                if (mro_contains((void *)sublist_slice, head))
                    contains_head = true;
                mro_delete((void *)sublist_slice);
            }
            mro_delete((void *)sequence_slice);

            if (!contains_head) {
                is_clean = true;
                break;
            }
        }

        if (! is_clean) {
            fprintf(stderr, "Inconsistent Hiearchy.\n");
            assert(0); 
            exit(EXIT_FAILURE); // incase assertion is compiled away.
        }
        
        // move head from front of seq to end of result
        mro_append(result, head);    
        for (int i = 0; i < COUNT(sequence); i++) {
            const void * subseq = mro_get(sequence, i);

            if (mro_contains((void *)subseq, head)) {
                int idx = mro_index((void *)subseq, head);
                mro_pop((void *)subseq, idx);
            }
        }
        mro_delete(copy);
    }
}

static void delete_results(void * results)
{
    for (int i = 0; i < COUNT(results); i++)
        mro_delete((void *) mro_get(results, i));
    mro_delete(results);
}
static void * linearize(void * _class, void * result)
{   
    struct class * class = _class;
    struct mro_util * stack = mro_new(NULL);
    void * results = mro_new(NULL);

    add_to_graph(class, stack);
    // reverse sort
    qsort(stack->elements, (size_t) COUNT(stack), sizeof(void *), compare_len);
    for (int i = 0; i < COUNT(stack); i++) {
        const struct class * cls = mro_get(stack, i);
        if (cls == class)
            continue;
        void * cls_res = mro_new((void *) cls);
        if (cls == Object()) {
            mro_append(cls_res, Object());
            mro_append(results, cls_res);
            continue;
        }
        for (int i = 0; i < cls->mro.count; i++)
            mro_append(cls_res, cls->mro.mro[i]);
        mro_append(results, cls_res);
    }

    void * sequence = mro_new(NULL);
    void * head_seq = mro_new(NULL);
    void * bases = mro_new(NULL);

    mro_append(head_seq, class);
    mro_append(sequence, head_seq);
    
    for (int i = 0; i < class->super.count; i++) {
        mro_append(sequence, mro_inner(results, class->super.super[i]));
        mro_append(bases, class->super.super[i]);
    }
    mro_append(sequence, bases);
    
    void * res = merge_sequence(sequence, result);


    mro_delete(head_seq);
    mro_delete(bases);
    mro_delete(sequence);
    mro_delete(stack);
    delete_results(results);
    
    return res;
}

static void create_mro(void * class, struct mro * mro)
{   
    void * result = linearize(class, mro_new(NULL));
    
    mro->count = COUNT(result);
    mro->mro = calloc(COUNT(result), sizeof(void *));
    for (int i = 0; i < COUNT(result); i++)
        mro->mro[i] = mro_get(result, i);
    mro_delete(result);
  
}

bool issubclass(const void * _class, const void * _superclass)
{
    void * graph = mro_new(NULL);
    add_to_graph(_class, graph);
    bool result =  mro_contains(graph, _superclass);
    mro_delete(graph);
    return result;
}
