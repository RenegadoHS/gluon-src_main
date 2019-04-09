#ifndef URIRESOLVER__H
#define URIRESOLVER__H

#ifndef XMLCH_DEFINED
	#define XMLCH_DEFINED
	typedef unsigned char XMLCH;
#endif

struct uriresolver_input_t *Uriresolver_RemoveInput(struct uriresolver_input_t *cur);
struct uriresolver_input_t *Uriresolver_AddInput(struct uriresolver_input_t *cur, XMLCH *uri);

struct uriresolver_input_t {
	void *src;
	XMLCH *base;
	size_t baselen;
	struct uriresolver_input_t *prev, *absolute;
};

#endif /* URIRESOLVER__H */

/*

Simple uri resolver methods for parsifal xml parser.
Provides relative/absolute/etc path handling for resolveEntity
/externalEntityParsed callbacks when complex dtd resources
need to be parsed.
  
struct uriresolver_input_t is intended to be used as 
XMLParser_Parse/ParseValidateDTD method's inputdata 
where src pointer should be set to point to open FILE* or
similar source or to be used as some other user defined
data in inputsource callbacks.

Usage:

You need uriresolver_input_t pointer to keep track current
inputsource between resolveEntity/externalEntityParsed calls
xmlplint uses global curinput variable since it's single
threaded app. Also you need stringbuffer/string for concatenating
possible base uri and filename.

static struct uriresolver_input_t *curinput;
static XMLSTRINGBUF uribuf;

also you need 1 uriresolver_input_t to mark the bottom
of the stack, xmlplint uses 

struct uriresolver_input_t startinp = { NULL, NULL, 0, NULL, &startinp };
curinput = &startinp;

setting curinput->src (curinput==&startinp) to stdin for example and setting
base to point to "/home" for example and baselen to strlen(curinput->base)
gives you curinput to use for XMLParser_Parse inputdata parameter.
Note that setting base and baselen is optional.

If you want to resolve (and store) base uri you then call:
curinput = Uriresolver_AddInput(curinput, "http://myserver/mydata.xml");

after AddInput call you test if
curinput->src == NULL which means this is absolute uri which you
can use as it is. If curinput->src points to somewhere for example
"subdir/mydata.xml" would make curinput-> src to point to
"mydata.xml" you must concatenate current absolute base dir and
curinput->src. Example from xmlplint resolveEntityHandler:

if (!curinput->src) { 
	uri = systemID;
}
else {
	if (uribuf.len) uribuf.len = 0;
	uri = XMLStringbuf_Append(&uribuf, curinput->absolute->base, curinput->absolute->baselen);
	ASSERT_MEM_ABORT(uri);
	uri = XMLStringbuf_Append(&uribuf, curinput->src, strlen(curinput->src)+1);
	ASSERT_MEM_ABORT(uri);
}

and then (NOTE the curinput->srs's twofold role as 'output' from AddInput
and as reader's inputData):

curinput->src = myopen(uri);
reader->inputData = curinput;

So in practice you need curinput->src, curinput->absolute->base and curinput->absolute->baselen
members of uriresolver_input_t only.

in externalEntityParsedHandler (xmlplint uses handler called FreeInputData)
you must remove the current inputdata from the stack by calling
Uriresolver_RemoveInput:

curinput = Uriresolver_RemoveInput((struct uriresolver_input_t*)reader->inputData);

after parsing you can clean up the stack by iterating:

while (curinput != &startinp) {
	curinput = Uriresolver_RemoveInput(curinput);
}

(actually parsifal calls externalEntityParsedHandler in error conditions too
but since inputstack can have the main document added by AddInput call
this clean up is necessary)

See also catalogs.c and PrepNewInput() + ResolveEntity()

*/

