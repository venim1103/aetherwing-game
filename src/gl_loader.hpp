#pragma once
#define GL_GLEXT_PROTOTYPES 0
#include <GL/gl.h>
#include <GL/glext.h>
#include <cstdint>

namespace glfn {

extern PFNGLGENVERTEXARRAYSPROC          GenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC          BindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC       DeleteVertexArrays;
extern PFNGLGENBUFFERSPROC               GenBuffers;
extern PFNGLBINDBUFFERPROC               BindBuffer;
extern PFNGLBUFFERDATAPROC               BufferData;
extern PFNGLBUFFERSUBDATAPROC            BufferSubData;
extern PFNGLDELETEBUFFERSPROC            DeleteBuffers;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC  EnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC      VertexAttribPointer;
extern PFNGLCREATESHADERPROC             CreateShader;
extern PFNGLSHADERSOURCEPROC             ShaderSource;
extern PFNGLCOMPILESHADERPROC            CompileShader;
extern PFNGLGETSHADERIVPROC              GetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC         GetShaderInfoLog;
extern PFNGLDELETESHADERPROC             DeleteShader;
extern PFNGLCREATEPROGRAMPROC            CreateProgram;
extern PFNGLATTACHSHADERPROC             AttachShader;
extern PFNGLLINKPROGRAMPROC              LinkProgram;
extern PFNGLGETPROGRAMIVPROC             GetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC        GetProgramInfoLog;
extern PFNGLDELETEPROGRAMPROC            DeleteProgram;
extern PFNGLUSEPROGRAMPROC               UseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC       GetUniformLocation;
extern PFNGLUNIFORM1IPROC                Uniform1i;
extern PFNGLUNIFORM1FPROC                Uniform1f;
extern PFNGLUNIFORM2FPROC                Uniform2f;
extern PFNGLUNIFORM3FPROC                Uniform3f;
extern PFNGLUNIFORM4FPROC                Uniform4f;
extern PFNGLUNIFORM3FVPROC               Uniform3fv;
extern PFNGLUNIFORMMATRIX4FVPROC         UniformMatrix4fv;
extern PFNGLACTIVETEXTUREPROC            ActiveTexture;
extern PFNGLGENFRAMEBUFFERSPROC          GenFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC          BindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC       DeleteFramebuffers;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC     FramebufferTexture2D;
extern PFNGLGENRENDERBUFFERSPROC         GenRenderbuffers;
extern PFNGLBINDRENDERBUFFERPROC         BindRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC      DeleteRenderbuffers;
extern PFNGLRENDERBUFFERSTORAGEPROC      RenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC  FramebufferRenderbuffer;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC   CheckFramebufferStatus;
extern PFNGLGENERATEMIPMAPPROC           GenerateMipmap;
extern PFNGLBLENDFUNCSEPARATEPROC        BlendFuncSeparate;

bool loadAll();
GLuint compileProgram(const char* vertSrc, const char* fragSrc, const char* debugName);

} // namespace glfn
