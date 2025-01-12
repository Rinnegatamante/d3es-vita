/*
Emile Belanger
GPL3
*/
#include "renderer/tr_local.h"
#include "renderer/VertexCache.h"


idCVar r_framebufferFilter( "r_framebufferFilter", "0", CVAR_RENDERER | CVAR_BOOL, "Image filter when using the framebuffer. 0 = Nearest, 1 = Linear" );

static GLuint m_framebuffer = -1;
static GLuint m_depthbuffer;
static GLuint m_stencilbuffer;

static int m_framebuffer_width, m_framebuffer_height;
static GLuint m_framebuffer_texture;

static GLuint m_positionLoc;
static GLuint m_texCoordLoc;
static GLuint m_samplerLoc;

static GLuint r_program;

static int fixNpot(int v)
{
	int ret = 1;
	while(ret < v)
		ret <<= 1;
	return ret;
}

#define LOG common->Printf
int loadShader(int shaderType, const char * source)
{
	int shader = qglCreateShader(shaderType);

	if(shader != 0)
	{
		qglShaderSource(shader, 1, &source, NULL);
		qglCompileShader(shader);

		GLint  length;

		qglGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		if(length)
		{
			char* buffer  =  new char [ length ];
			qglGetShaderInfoLog(shader, length, NULL, buffer);
			LOG("shader = %s\n", buffer);
			delete [] buffer;

			GLint success;
			qglGetShaderiv(shader, GL_COMPILE_STATUS, &success);

			if(success != GL_TRUE)
			{
				LOG("ERROR compiling shader\n");
			}
		}
	}
	else
	{
		LOG("FAILED to create shader");
	}

	return shader;
}

int createProgram(const char * vertexSource, const char *  fragmentSource)
{
	int vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
	int pixelShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);

	int program = qglCreateProgram();

	if(program != 0)
	{
		qglAttachShader(program, vertexShader);
		// checkGlError("glAttachShader");
		qglAttachShader(program, pixelShader);
		// checkGlError("glAttachShader");
		qglLinkProgram(program);
#define GL_LINK_STATUS 0x8B82
		int linkStatus[1];
		qglGetProgramiv(program, GL_LINK_STATUS, linkStatus);

		if(linkStatus[0] != GL_TRUE)
		{
			LOG("Could not link program: ");
			char log[256];
			GLsizei size;
			qglGetProgramInfoLog(program, 256, &size, log);
			LOG("Log: %s", log);
			//glDeleteProgram(program);
			program = 0;
		}

	}
	else
	{
		LOG("FAILED to create program");
	}

	LOG("Program linked OK %d", program);
	return program;
}

static void createShaders (void)
{
	const GLchar *vertSource = \
           "void main(float4 a_position, float2 a_texCoord,                \n \
			    float2 out v_texCoord : TEXCOORD0,                         \n \
			    float4 out gl_Position : POSITION) {                       \n \
			   gl_Position = a_position;                                   \n \
			   v_texCoord = a_texCoord;                                    \n \
			}                                                              \n \
			";

	const GLchar *fragSource = \
			"float4 main(float2 v_texCoord : TEXCOORD0,              \n \
			uniform sampler2D s_texture)                             \n  \
			{                                                        \n  \
				return tex2D( s_texture, v_texCoord );               \n  \
			}                                                        \n  \
			";


	r_program = createProgram(vertSource, fragSource);

	qglUseProgram(r_program);

   // get attrib locations
	m_positionLoc = qglGetAttribLocation(r_program, "a_position");
	m_texCoordLoc = qglGetAttribLocation(r_program, "a_texCoord");
	m_samplerLoc  = qglGetUniformLocation(r_program, "s_texture");

	if(m_positionLoc == -1)
		LOG("Failed to get m_positionLoc");

	if(m_texCoordLoc == -1)
		LOG("Failed to get m_texCoordLoc");

	if(m_samplerLoc == -1)
		LOG("Failed to get m_samplerLoc");

	qglUniform1i(m_samplerLoc, 0);
}


void R_InitFrameBuffer()
{
	LOG("R_InitFrameBuffer Real[%d, %d] -> Framebuffer[%d,%d]", glConfig.vidWidthReal, glConfig.vidHeightReal, glConfig.vidWidth, glConfig.vidHeight);

	if(glConfig.vidWidthReal == glConfig.vidWidth && glConfig.vidHeightReal == glConfig.vidHeight)
	{
		LOGI("Not using framebuffer");
		return;
	}

	glConfig.npotAvailable = false;

	m_framebuffer_width = glConfig.vidWidth;
	m_framebuffer_height = glConfig.vidHeight;

	if (!glConfig.npotAvailable)
	{
		m_framebuffer_width = fixNpot(m_framebuffer_width);
		m_framebuffer_height = fixNpot(m_framebuffer_height);
	}

	LOGI("Framebuffer buffer size = [%d, %d]", m_framebuffer_width, m_framebuffer_height);

	// Create texture
	qglGenTextures(1, &m_framebuffer_texture);
	qglBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);
	
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_framebuffer_width, m_framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	if(r_framebufferFilter.GetInteger() == 0)
	{
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// Create framebuffer
	qglGenFramebuffers(1, &m_framebuffer);

	// Create renderbuffer
	/*qglGenRenderbuffers(1, &m_depthbuffer);
	qglBindRenderbuffer(GL_RENDERBUFFER, m_depthbuffer);

	if(glConfig.depthStencilAvailable)
		qglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, m_framebuffer_width, m_framebuffer_height);
	else
	{
		qglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_framebuffer_width, m_framebuffer_height);

		// Need separate Stencil buffer
		qglGenRenderbuffers(1, &m_stencilbuffer);
		qglBindRenderbuffer(GL_RENDERBUFFER, m_stencilbuffer);
        qglRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, m_framebuffer_width, m_framebuffer_height);
	}*/

	createShaders();
}

void R_FrameBufferStart()
{
	if(m_framebuffer == -1)
		return;

	// Render to framebuffer
    qglBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
   // qglBindRenderbuffer(GL_RENDERBUFFER, 0);

    qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_framebuffer_texture, 0);

    // Attach combined depth+stencil
    /*if(glConfig.depthStencilAvailable)
    {
    	qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer);
    	qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer);
	}
	else
	{
		qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer);
		qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_stencilbuffer);
	}*/

    GLenum result = qglCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(result != GL_FRAMEBUFFER_COMPLETE)
	{
	    common->Error( "Error binding Framebuffer: %d\n", result );
	}
}


void R_FrameBufferEnd()
{
	if(m_framebuffer == -1)
		return;

	qglBindFramebuffer(GL_FRAMEBUFFER, 0);
	qglBindRenderbuffer(GL_RENDERBUFFER, 0);

	// Bind the texture
	qglBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);

	// Unbind any VBOs
	vertexCache.UnbindIndex();
	vertexCache.UnbindVertex();

	qglUseProgram(r_program);

	GLfloat vert[] =
	{
		-1.f, -1.f,  0.0f,  // 0. left-bottom
		-1.f,  1.f,  0.0f,  // 1. left-top
		1.f, 1.f,  0.0f,    // 2. right-top
		1.f, -1.f,  0.0f,   // 3. right-bottom
	};

	GLfloat smax = 1;
	GLfloat tmax = 1;

	if (!glConfig.npotAvailable)
	{
		smax =  (float)glConfig.vidWidth / (float)m_framebuffer_width;
		tmax =   (float)glConfig.vidHeight / (float)m_framebuffer_height;
	}

	GLfloat texVert[] =
	{
		0.0f, 0.0f, // TexCoord 0
		0.0f, tmax, // TexCoord 1
		smax, tmax, // TexCoord 2
		smax, 0.0f  // TexCoord 3
	};

	qglVertexAttribPointer(m_positionLoc, 3, GL_FLOAT,
				  false,
				  3 * 4,
				  vert);

	qglVertexAttribPointer(m_texCoordLoc, 2, GL_FLOAT,
						  false,
						  2 * 4,
						  texVert);

	qglEnableVertexAttribArray(m_positionLoc);
	qglEnableVertexAttribArray(m_texCoordLoc);


	// Set the sampler texture unit to 0
	qglUniform1i(m_samplerLoc, 0);

	qglViewport (0, 0, glConfig.vidWidthReal, glConfig.vidHeightReal );

	qglDisable(GL_BLEND);
	qglDisable(GL_SCISSOR_TEST);
	qglDisable(GL_DEPTH_TEST);

	qglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


