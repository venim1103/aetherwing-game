#include "gl_loader.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <vector>

namespace glfn {

PFNGLGENVERTEXARRAYSPROC          GenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC          BindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC       DeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC               GenBuffers = nullptr;
PFNGLBINDBUFFERPROC               BindBuffer = nullptr;
PFNGLBUFFERDATAPROC               BufferData = nullptr;
PFNGLBUFFERSUBDATAPROC            BufferSubData = nullptr;
PFNGLDELETEBUFFERSPROC            DeleteBuffers = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC  EnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC      VertexAttribPointer = nullptr;
PFNGLCREATESHADERPROC             CreateShader = nullptr;
PFNGLSHADERSOURCEPROC             ShaderSource = nullptr;
PFNGLCOMPILESHADERPROC            CompileShader = nullptr;
PFNGLGETSHADERIVPROC              GetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC         GetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC             DeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC            CreateProgram = nullptr;
PFNGLATTACHSHADERPROC             AttachShader = nullptr;
PFNGLLINKPROGRAMPROC              LinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC             GetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC        GetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC            DeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC               UseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC       GetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC                Uniform1i = nullptr;
PFNGLUNIFORM1FPROC                Uniform1f = nullptr;
PFNGLUNIFORM2FPROC                Uniform2f = nullptr;
PFNGLUNIFORM3FPROC                Uniform3f = nullptr;
PFNGLUNIFORM4FPROC                Uniform4f = nullptr;
PFNGLUNIFORM3FVPROC               Uniform3fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC         UniformMatrix4fv = nullptr;
PFNGLACTIVETEXTUREPROC            ActiveTexture = nullptr;
PFNGLGENFRAMEBUFFERSPROC          GenFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC          BindFramebuffer = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC       DeleteFramebuffers = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC     FramebufferTexture2D = nullptr;
PFNGLGENRENDERBUFFERSPROC         GenRenderbuffers = nullptr;
PFNGLBINDRENDERBUFFERPROC         BindRenderbuffer = nullptr;
PFNGLDELETERENDERBUFFERSPROC      DeleteRenderbuffers = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC      RenderbufferStorage = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC  FramebufferRenderbuffer = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC   CheckFramebufferStatus = nullptr;
PFNGLGENERATEMIPMAPPROC           GenerateMipmap = nullptr;
PFNGLBLENDFUNCSEPARATEPROC        BlendFuncSeparate = nullptr;

template <typename T>
static bool loadProc(T& fn, const char* name) {
    fn = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!fn) {
        std::fprintf(stderr, "[GL] Missing required OpenGL function: %s\n", name);
        return false;
    }
    return true;
}

bool loadAll() {
    bool ok = true;
    ok &= loadProc(GenVertexArrays,         "glGenVertexArrays");
    ok &= loadProc(BindVertexArray,         "glBindVertexArray");
    ok &= loadProc(DeleteVertexArrays,      "glDeleteVertexArrays");
    ok &= loadProc(GenBuffers,              "glGenBuffers");
    ok &= loadProc(BindBuffer,              "glBindBuffer");
    ok &= loadProc(BufferData,              "glBufferData");
    ok &= loadProc(BufferSubData,           "glBufferSubData");
    ok &= loadProc(DeleteBuffers,           "glDeleteBuffers");
    ok &= loadProc(EnableVertexAttribArray, "glEnableVertexAttribArray");
    ok &= loadProc(VertexAttribPointer,     "glVertexAttribPointer");
    ok &= loadProc(CreateShader,            "glCreateShader");
    ok &= loadProc(ShaderSource,            "glShaderSource");
    ok &= loadProc(CompileShader,           "glCompileShader");
    ok &= loadProc(GetShaderiv,             "glGetShaderiv");
    ok &= loadProc(GetShaderInfoLog,        "glGetShaderInfoLog");
    ok &= loadProc(DeleteShader,            "glDeleteShader");
    ok &= loadProc(CreateProgram,           "glCreateProgram");
    ok &= loadProc(AttachShader,            "glAttachShader");
    ok &= loadProc(LinkProgram,             "glLinkProgram");
    ok &= loadProc(GetProgramiv,            "glGetProgramiv");
    ok &= loadProc(GetProgramInfoLog,       "glGetProgramInfoLog");
    ok &= loadProc(DeleteProgram,           "glDeleteProgram");
    ok &= loadProc(UseProgram,              "glUseProgram");
    ok &= loadProc(GetUniformLocation,      "glGetUniformLocation");
    ok &= loadProc(Uniform1i,               "glUniform1i");
    ok &= loadProc(Uniform1f,               "glUniform1f");
    ok &= loadProc(Uniform2f,               "glUniform2f");
    ok &= loadProc(Uniform3f,               "glUniform3f");
    ok &= loadProc(Uniform4f,               "glUniform4f");
    ok &= loadProc(Uniform3fv,              "glUniform3fv");
    ok &= loadProc(UniformMatrix4fv,        "glUniformMatrix4fv");
    ok &= loadProc(ActiveTexture,           "glActiveTexture");
    ok &= loadProc(GenFramebuffers,         "glGenFramebuffers");
    ok &= loadProc(BindFramebuffer,         "glBindFramebuffer");
    ok &= loadProc(DeleteFramebuffers,      "glDeleteFramebuffers");
    ok &= loadProc(FramebufferTexture2D,    "glFramebufferTexture2D");
    ok &= loadProc(GenRenderbuffers,        "glGenRenderbuffers");
    ok &= loadProc(BindRenderbuffer,        "glBindRenderbuffer");
    ok &= loadProc(DeleteRenderbuffers,     "glDeleteRenderbuffers");
    ok &= loadProc(RenderbufferStorage,     "glRenderbufferStorage");
    ok &= loadProc(FramebufferRenderbuffer, "glFramebufferRenderbuffer");
    ok &= loadProc(CheckFramebufferStatus,  "glCheckFramebufferStatus");
    ok &= loadProc(GenerateMipmap,          "glGenerateMipmap");
    ok &= loadProc(BlendFuncSeparate,       "glBlendFuncSeparate");
    return ok;
}

static GLuint compileStage(GLenum type, const char* src, const char* debugName) {
    GLuint s = CreateShader(type);
    ShaderSource(s, 1, &src, nullptr);
    CompileShader(s);
    GLint ok = 0;
    GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        GetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 2, 0);
        GetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "[GL] Shader compile error (%s, %s):\n%s\n",
                     debugName, type == GL_VERTEX_SHADER ? "VERT" : "FRAG", log.data());
        DeleteShader(s);
        return 0;
    }
    return s;
}

GLuint compileProgram(const char* vertSrc, const char* fragSrc, const char* debugName) {
    GLuint vs = compileStage(GL_VERTEX_SHADER, vertSrc, debugName);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragSrc, debugName);
    if (!vs || !fs) {
        if (vs) DeleteShader(vs);
        if (fs) DeleteShader(fs);
        return 0;
    }
    GLuint prog = CreateProgram();
    AttachShader(prog, vs);
    AttachShader(prog, fs);
    LinkProgram(prog);
    DeleteShader(vs);
    DeleteShader(fs);

    GLint ok = 0;
    GetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        GetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 2, 0);
        GetProgramInfoLog(prog, len, nullptr, log.data());
        std::fprintf(stderr, "[GL] Program link error (%s):\n%s\n", debugName, log.data());
        DeleteProgram(prog);
        return 0;
    }
    return prog;
}

} // namespace glfn
