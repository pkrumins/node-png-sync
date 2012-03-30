#include <cstring>
#include <cstdlib>
#include "common.h"
#include "png_encoder.h"
#include "png.h"
#include "buffer_compat.h"

using namespace v8;
using namespace node;

void
Png::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", PngEncodeSync);
    target->Set(String::NewSymbol("Png"), t->GetFunction());
}

Png::Png(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type) {}

Handle<Value>
Png::PngEncodeSync()
{
    HandleScope scope;

    Local<Value> buf_val = handle_->GetHiddenValue(String::New("buffer"));

    char *buf_data = BufferData(buf_val->ToObject());

    try {
        PngEncoder encoder((unsigned char*)buf_data, width, height, buf_type);
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
Png::New(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() < 3)
        return VException("At least three arguments required - data buffer, width, height, [and input buffer type]");
    if (!Buffer::HasInstance(args[0]))
        return VException("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer width.");
    if (!args[2]->IsInt32())
        return VException("Third argument must be integer height.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 4) {
        if (!args[3]->IsString())
            return VException("Fourth argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[3]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return VException("Fourth argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
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
            return VException("Fourth argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }


    int w = args[1]->Int32Value();
    int h = args[2]->Int32Value();

    if (w < 0)
        return VException("Width smaller than 0.");
    if (h < 0)
        return VException("Height smaller than 0.");

    Png *png = new Png(w, h, buf_type);
    png->Wrap(args.This());

    // Save buffer.
    png->handle_->SetHiddenValue(String::New("buffer"), args[0]);

    return args.This();
}

Handle<Value>
Png::PngEncodeSync(const Arguments &args)
{
    HandleScope scope;
    Png *png = ObjectWrap::Unwrap<Png>(args.This());
    return scope.Close(png->PngEncodeSync());
}

