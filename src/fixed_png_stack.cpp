#include <cstdlib>

#include "png_encoder.h"
#include "fixed_png_stack.h"
#include "buffer_compat.h"

using namespace v8;
using namespace node;

void
FixedPngStack::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", PngEncodeSync);
    target->Set(String::NewSymbol("FixedPngStack"), t->GetFunction());
}

FixedPngStack::FixedPngStack(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type)
{ 
    data = (unsigned char *)malloc(sizeof(*data) * width * height * 4);
    if (!data) throw "malloc failed in node-png (FixedPngStack ctor)";
    memset(data, 0xFF, width*height*4);
}

FixedPngStack::~FixedPngStack()
{
    free(data);
}

void
FixedPngStack::Push(unsigned char *buf_data, int x, int y, int w, int h)
{
    int start = y*width*4 + x*4;
    for (int i = 0; i < h; i++) {
        unsigned char *datap = &data[start + i*width*4];
        for (int j = 0; j < w; j++) {
            *datap++ = *buf_data++;
            *datap++ = *buf_data++;
            *datap++ = *buf_data++;
            *datap++ = (buf_type == BUF_RGB || buf_type == BUF_BGR) ? 0x00 : *buf_data++;
        }
    }
}

Handle<Value>
FixedPngStack::PngEncodeSync()
{
    HandleScope scope;

    buffer_type pbt = (buf_type == BUF_BGR || buf_type == BUF_BGRA) ? BUF_BGRA : BUF_RGBA;

    try {
        PngEncoder encoder(data, width, height, pbt);
        encoder.encode();
        int png_len = encoder.get_png_len();
        Buffer *retbuf = Buffer::New(png_len);
        memcpy(BufferData(retbuf), encoder.get_png(), png_len);
        return scope.Close(retbuf->handle_);
    }
    catch (const char *err) {
        return VException(err);
    }
}

Handle<Value>
FixedPngStack::New(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() < 2)
        return VException("At least two arguments required - width and height [and input buffer type].");
    if (!args[0]->IsInt32())
        return VException("First argument must be integer width.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer height.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 3) {
        if (!args[2]->IsString())
            return VException("Third argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[2]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
                    str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return VException("Third argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
        }

        if (str_eq(*bts, "rgb"))
            buf_type = BUF_RGB;
        else if (str_eq(*bts, "bgr"))
            buf_type = BUF_BGR;
        else if (str_eq(*bts, "rgba"))
            buf_type = BUF_RGBA;
        else if (str_eq(*bts, "bgra"))
            buf_type = BUF_BGRA;
        else
            return VException("Third argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    int width = args[0]->Int32Value();
    int height = args[1]->Int32Value();

    try {
        FixedPngStack *png_stack = new FixedPngStack(width, height, buf_type);
        png_stack->Wrap(args.This());
        return args.This();
    }
    catch (const char *e) {
        return VException(e);
    }
}

Handle<Value>
FixedPngStack::Push(const Arguments &args)
{
    HandleScope scope;

    if (!Buffer::HasInstance(args[0]))
        return VException("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer x.");
    if (!args[2]->IsInt32())
        return VException("Third argument must be integer y.");
    if (!args[3]->IsInt32())
        return VException("Fourth argument must be integer w.");
    if (!args[4]->IsInt32())
        return VException("Fifth argument must be integer h.");

    FixedPngStack *png_stack = ObjectWrap::Unwrap<FixedPngStack>(args.This());
    int x = args[1]->Int32Value();
    int y = args[2]->Int32Value();
    int w = args[3]->Int32Value();
    int h = args[4]->Int32Value();

    if (x < 0)
        return VException("Coordinate x smaller than 0.");
    if (y < 0)
        return VException("Coordinate y smaller than 0.");
    if (w < 0)
        return VException("Width smaller than 0.");
    if (h < 0)
        return VException("Height smaller than 0.");
    if (x >= png_stack->width) 
        return VException("Coordinate x exceeds FixedPngStack's dimensions.");
    if (y >= png_stack->height) 
        return VException("Coordinate y exceeds FixedPngStack's dimensions.");
    if (x+w > png_stack->width) 
        return VException("Pushed PNG exceeds FixedPngStack's width.");
    if (y+h > png_stack->height) 
        return VException("Pushed PNG exceeds FixedPngStack's height.");

    char *buf_data = BufferData(args[0]->ToObject());

    png_stack->Push((unsigned char*)buf_data, x, y, w, h);

    return Undefined();
}

Handle<Value>
FixedPngStack::PngEncodeSync(const Arguments &args)
{
    HandleScope scope;

    FixedPngStack *png_stack = ObjectWrap::Unwrap<FixedPngStack>(args.This());
    return png_stack->PngEncodeSync();
}

