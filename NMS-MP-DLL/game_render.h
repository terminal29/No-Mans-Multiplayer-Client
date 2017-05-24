#pragma once

#include <gl/gl3w.h>
#include <glm/common.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glcorearb.h>
#include "game_value.h"
#include <vector>
#include "lodepng.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			Namespace for all rendering functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace game_render
{
	unsigned int atlas_tex_w = 1024, atlas_tex_h = 1024;
	std::vector<unsigned char> atlas_tex;



	static void render_player_marker(float x, float y, float z) {
		/*if (atlas_tex_w == -1 || atlas_tex_h == -1) {
			MessageBox(NULL, "Load image", "", MB_OK);
			unsigned int texDecode = lodepng::decode(atlas_tex, atlas_tex_w, atlas_tex_h, std::string("C:\\Program Files (x86)\\GalaxyClient\\Games\\No Man's Sky\\Binaries\\Atlas.png"));
			if (texDecode) {
				MessageBox(NULL, lodepng_error_text(texDecode), "", MB_OK);
				atlas_tex_w = 0;
				atlas_tex_h = 0;
			}
		}*/
		while (atlas_tex.size() < atlas_tex_w * atlas_tex_h * 4) {
			atlas_tex.push_back((unsigned char)128);
		}


		float player_position[3];
		game_value::get_player_position(player_position);

		float player_rotation_vectors[9];
		game_value::get_player_rotation_vectors(player_rotation_vectors);


		RECT window_rect;
		GetWindowRect(WindowFromDC(wglGetCurrentDC()), &window_rect);
		int w = window_rect.right - window_rect.left;
		int h = window_rect.bottom - window_rect.top;

		glm::vec3 Xax = { player_rotation_vectors[0], player_rotation_vectors[1], player_rotation_vectors[2] };
		glm::vec3 Yax = { -player_rotation_vectors[3], -player_rotation_vectors[4], -player_rotation_vectors[5] };
		glm::vec3 Zax = { -player_rotation_vectors[6], -player_rotation_vectors[7], -player_rotation_vectors[8] };
		glm::vec3 pos = { player_position[0], player_position[1], player_position[2] };

		glm::mat4 View = { Xax.x, Yax.x, Zax.x, 0,
			Xax.y, Yax.y, Zax.y, 0,
			Xax.z, Yax.z, Zax.z, 0,
			-glm::dot(pos, Xax), -glm::dot(pos, Yax), -glm::dot(pos, Zax), 1 };


		glm::mat4 projectionMatrix = glm::perspective(
			glm::radians(45.0f),         // The horizontal Field of View, in degrees : the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
			(float)w / h, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
			0.1f,        // Near clipping plane. Keep as big as possible, or you'll get precision issues.
			1000.0f       // Far clipping plane. Keep as little as possible.
		);

		// Model matrix : an identity matrix (model will be at the origin)
		glm::mat4 Model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
		//Model = glm::lookAt(glm::vec3(x, y, z), glm::vec3(me.getPos()[0], me.getPos()[1], me.getPos()[2]), glm::vec3(0, 1, 0));

		//Model = glm::translate(Model, glm::vec3(0, 0, -1));
		// Our ModelViewProjection : multiplication of our 3 matrices
		glm::mat4 modelviewMatrix = View * Model; // Remember, matrix multiplication is the other way around

		GLchar* vertexsource = "#version 410 compatibility\n	\
		uniform mat4 MV;							\
		uniform mat4 P;								\
		uniform sampler2D tex;						\
		in  vec3 in_Position;						\
		void main(void) {							\
			gl_Position = P * (MV * vec4(0.0, 0.0, 0.0, 1.0) + vec4(in_Position.x, in_Position.y, 0.0, 0.0));\
		}";

		GLchar* fragmentsource = "#version 410 compatibility\n	\
		precision highp float;						\
													\
		in  vec2 uv;								\
		uniform sampler2D tex;						\
		out vec4 gl_FragColor;						\
													\
		void main(void) {							\
			gl_FragColor = texture(tex,uv) ;		\
		}";

		int i; /* Simple iterator */
		GLuint vao, vbo[2]; /* Create handles for our Vertex Array Object and two Vertex Buffer Objects */
		int IsCompiled_VS, IsCompiled_FS;
		int IsLinked;
		int maxLength;
		char *vertexInfoLog;
		char *fragmentInfoLog;
		char *shaderProgramInfoLog;

		float scale = 0.75;

		/* We're going to create a simple diamond made from lines */
		const GLfloat diamond[4][2] = {
			{ -scale, -scale }, /* Top point */
			{ scale, -scale }, /* Right point */
			{ scale, scale }, /* Bottom point */
			{ -scale, scale } }; /* Left point */

		const GLfloat uv[4][3] = {
			{ 0.0,  0.0 },
			{ 0.0,  1.0 },
			{ 1.0,  1.0 },
			{ 0.0,  1.0 } };

		glEnable(GL_TEXTURE_2D);
		GLuint atlas_tex_id;
		glGenTextures(1, &atlas_tex_id);
		glBindTexture(GL_TEXTURE_2D, atlas_tex_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas_tex_w, atlas_tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &atlas_tex[0]); 
		glBindTexture(GL_TEXTURE_2D, 0);
		/* These are handles used to reference the shaders */
		GLuint vertexshader, fragmentshader;

		/* This is a handle to the shader program */
		GLuint shaderprogram;

		/* Allocate and assign a Vertex Array Object to our handle */
		glGenVertexArrays(1, &vao);

		/* Bind our Vertex Array Object as the current used object */
		glBindVertexArray(vao);

		/* Allocate and assign two Vertex Buffer Objects to our handle */
		glGenBuffers(2, vbo);

		/* Bind our first VBO as being the active buffer and storing vertex attributes (coordinates) */
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

		/* Copy the vertex data from diamond to our buffer */
		/* 8 * sizeof(GLfloat) is the size of the diamond array, since it contains 8 GLfloat values */
		glBufferData(GL_ARRAY_BUFFER, sizeof(diamond), diamond, GL_STATIC_DRAW);

		/* Specify that our coordinate data is going into attribute index 0, and contains two floats per vertex */
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

		/* Enable attribute index 0 as being used */
		glEnableVertexAttribArray(0);

		/* Bind our second VBO as being the active buffer and storing vertex attributes (colors) */
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

		/* Copy the color data from colors to our buffer */
		/* 12 * sizeof(GLfloat) is the size of the colors array, since it contains 12 GLfloat values */
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

		/* Specify that our color data is going into attribute index 1, and contains three floats per vertex */
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		/* Enable attribute index 1 as being used */
		glEnableVertexAttribArray(1);

		/* Create an empty vertex shader handle */
		vertexshader = glCreateShader(GL_VERTEX_SHADER);

		/* Send the vertex shader source code to GL */
		/* Note that the source code is NULL character terminated. */
		/* GL will automatically detect that therefore the length info can be 0 in this case (the last parameter) */
		glShaderSource(vertexshader, 1, (const GLchar**)&vertexsource, 0);

		/* Compile the vertex shader */
		glCompileShader(vertexshader);

		glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &IsCompiled_VS);
		if (IsCompiled_VS == FALSE)
		{
			glGetShaderiv(vertexshader, GL_INFO_LOG_LENGTH, &maxLength);

			/* The maxLength includes the NULL character */
			vertexInfoLog = (char *)malloc(maxLength);

			glGetShaderInfoLog(vertexshader, maxLength, &maxLength, vertexInfoLog);

			/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
			/* In this simple program, we'll just leave */
			MessageBoxA(NULL, vertexInfoLog, NULL, MB_OK);
			free(vertexInfoLog);
		}

		/* Create an empty fragment shader handle */
		fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);


		/* Compile the fragment shader */
		glCompileShader(fragmentshader);

		glGetShaderiv(fragmentshader, GL_COMPILE_STATUS, &IsCompiled_FS);
		if (IsCompiled_FS == FALSE)
		{
			glGetShaderiv(fragmentshader, GL_INFO_LOG_LENGTH, &maxLength);
			fragmentInfoLog = (char *)malloc(maxLength);
			glGetShaderInfoLog(fragmentshader, maxLength, &maxLength, fragmentInfoLog);
			MessageBoxA(NULL, fragmentInfoLog, NULL, MB_OK);
			free(fragmentInfoLog);
		}

		/* If we reached this point it means the vertex and fragment shaders compiled and are syntax error free. */
		/* We must link them together to make a GL shader program */
		/* GL shader programs are monolithic. It is a single piece made of 1 vertex shader and 1 fragment shader. */
		/* Assign our program handle a "name" */
		shaderprogram = glCreateProgram();

		/* Attach our shaders to our program */
		glAttachShader(shaderprogram, vertexshader);
		glAttachShader(shaderprogram, fragmentshader);

		/* Bind attribute index 0 (coordinates) to in_Position and attribute index 1 (color) to in_Color */
		/* Attribute locations must be setup before calling glLinkProgram. */
		glBindAttribLocation(shaderprogram, 0, "in_Position");
		glBindAttribLocation(shaderprogram, 1, "uv");
		
		

		/* Link our program */
		/* At this stage, the vertex and fragment programs are inspected, optimized and a binary code is generated for the shader. */
		/* The binary code is uploaded to the GPU, if there is no error. */
		glLinkProgram(shaderprogram);

		/* Again, we must check and make sure that it linked. If it fails, it would mean either there is a mismatch between the vertex */
		/* and fragment shaders. It might be that you have surpassed your GPU's abilities. Perhaps too many ALU operations or */
		/* too many texel fetch instructions or too many interpolators or dynamic loops. */

		glGetProgramiv(shaderprogram, GL_LINK_STATUS, (int *)&IsLinked);
		if (IsLinked == FALSE)
		{
			/* Noticed that glGetProgramiv is used to get the length for a shader program, not glGetShaderiv. */
			glGetProgramiv(shaderprogram, GL_INFO_LOG_LENGTH, &maxLength);

			/* The maxLength includes the NULL character */
			shaderProgramInfoLog = (char *)malloc(maxLength);

			/* Notice that glGetProgramInfoLog, not glGetShaderInfoLog. */
			glGetProgramInfoLog(shaderprogram, maxLength, &maxLength, shaderProgramInfoLog);

			/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
			/* In this simple program, we'll just leave */
			free(shaderProgramInfoLog);
			MessageBoxA(NULL, shaderProgramInfoLog, NULL, MB_OK);
		}

		/* Load the shader into the rendering pipeline */
		glUseProgram(shaderprogram);

		// Update the uniforms AFTER glUseProgram
		GLuint modelview_handle = glGetUniformLocation(shaderprogram, "MV");
		GLuint projection_handle = glGetUniformLocation(shaderprogram, "P");
		glUniformMatrix4fv(modelview_handle, 1, GL_FALSE, &modelviewMatrix[0][0]);
		glUniformMatrix4fv(projection_handle, 1, GL_FALSE, &projectionMatrix[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas_tex_id);
		glUniform1i(glGetUniformLocation(shaderprogram, "tex"), 0);

		/* Invoke glDrawArrays telling that our data is a line loop and we want to draw 2-4 vertexes */
		glDrawArrays(GL_QUADS, 0, 4);

		/* Cleanup all the things we bound and allocated */
		//glUseProgram(0);
		glDeleteTextures(1, &atlas_tex_id);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDetachShader(shaderprogram, vertexshader);
		glDetachShader(shaderprogram, fragmentshader);
		glDeleteProgram(shaderprogram);
		glDeleteShader(vertexshader);
		glDeleteShader(fragmentshader);
		glDeleteBuffers(2, vbo);
		glDeleteVertexArrays(1, &vao);

	}
}