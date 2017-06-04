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
#include <Communication.h>
#include <vector>
#include "lodepng.h"
#include <chrono>
#include "debug.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			Namespace for all rendering functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace game_render
{
	unsigned int atlas_tex_w = 1024, atlas_tex_h = 1024;
	std::vector<unsigned char> atlas_tex;

	bool overlay_init_state = false;

	void render_player_marker(float x, float y, float z) {

		if (atlas_tex.size() == 0) {
			
			unsigned int texDecode = lodepng::decode(atlas_tex, atlas_tex_w, atlas_tex_h, std::string("C:\\Program Files (x86)\\GalaxyClient\\Games\\No Man's Sky\\Binaries\\Atlas.png"));

			if (texDecode) {
				display_error("Cannot load Atlas.png.\n Please place it in the same folder as your NMS.exe file.", false);
				atlas_tex_w = 0;
				atlas_tex_h = 0;
				return;
			}
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
		glm::vec3 pos = { player_position[0],  player_position[1],  player_position[2] };

		glm::mat4 view_matrix = { Xax.x, Yax.x, Zax.x, 0,Xax.y, Yax.y, Zax.y, 0,Xax.z, Yax.z, Zax.z, 0,-glm::dot(pos, Xax), -glm::dot(pos, Yax), -glm::dot(pos, Zax), 1 };
		glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), (float)w / h, 0.1f, 1000.0f );
		glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
		glm::mat4 model_view_matrix = view_matrix * model_matrix;

		GLchar* vertexsource = "#version 330\n\
		uniform mat4 model_view_mat;\
		uniform mat4 proj_mat;\
		\
		layout(location = 0) in vec3 pos;\
		layout(location = 1) in vec2 uv;\
		\
		out vec2 passthru_uv;\
		\
		void main(void) {\
			gl_Position = proj_mat * (model_view_mat * vec4(0.0, 0.0, 0.0, 1.0) + vec4(pos.x, pos.y, 0.0, 0.0));\
			passthru_uv = uv;\
		}";

		GLchar* fragmentsource = "#version 330\n\
		precision highp float;\
		\
		in vec2 passthru_uv;\
		uniform sampler2D tex;\
		out vec4 color;\
		\
		float pi = 3.14159265358979;\
		vec2 uv;\
		\
		void main(void) {\
			uv = passthru_uv;\
			uv = uv + vec2(-0.5,-0.5);\
			uv = mat2(cos(pi), sin(pi), -sin(pi), cos(pi)) * uv;\
			uv = uv + vec2(0.5,0.5);\
			color = texture2D(tex, uv.st);\
		}";

		GLuint vao, vbo[2];
		int IsCompiled_VS, IsCompiled_FS;
		int IsLinked;
		int maxLength;
		char *vertexInfoLog;
		char *fragmentInfoLog;
		char *shaderProgramInfoLog;

		float scale = 0.75 + 0.1 * std::sin((std::chrono::system_clock::now().time_since_epoch().count()/ 8000000.0) * 3.14 );

		/* We're going to create a simple diamond made from lines */
		const GLfloat square[6][2] = {
			{ -scale, -scale },
			{ scale, scale }, 
			{ scale, -scale },

			{ -scale, -scale },
			{ -scale, scale },
			{ scale, scale } };

			const GLfloat uv[6][2] = {
				{ 1, 0 },
				{ 0, 1 },
				{ 0, 0 },

				{ 1, 0 }, 
				{ 1, 1 }, 
				{ 0, 1 } };

		GLuint atlas_tex_id;
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &atlas_tex_id);
		glBindTexture(GL_TEXTURE_2D, atlas_tex_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas_tex_w, atlas_tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &atlas_tex[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(0);
		GLuint vertexshader, fragmentshader;

		GLuint shaderprogram;

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(2, vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		vertexshader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexshader, 1, (const GLchar**)&vertexsource, 0);
		glCompileShader(vertexshader);
		glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &IsCompiled_VS);
		if (IsCompiled_VS == FALSE)
		{
			glGetShaderiv(vertexshader, GL_INFO_LOG_LENGTH, &maxLength);
			vertexInfoLog = (char *)malloc(maxLength);
			glGetShaderInfoLog(vertexshader, maxLength, &maxLength, vertexInfoLog);
			MessageBoxA(NULL, vertexInfoLog, NULL, MB_OK);
			free(vertexInfoLog);
		}

		fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentshader, 1, (const GLchar**)&fragmentsource, 0);
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

		shaderprogram = glCreateProgram();

		glAttachShader(shaderprogram, vertexshader);
		glAttachShader(shaderprogram, fragmentshader);
		glBindAttribLocation(shaderprogram, 0, "pos");
		glBindAttribLocation(shaderprogram, 1, "uv");
		glLinkProgram(shaderprogram);
		glGetProgramiv(shaderprogram, GL_LINK_STATUS, (int *)&IsLinked);
		if (IsLinked == FALSE)
		{
			glGetProgramiv(shaderprogram, GL_INFO_LOG_LENGTH, &maxLength);
			shaderProgramInfoLog = (char *)malloc(maxLength);
			glGetProgramInfoLog(shaderprogram, maxLength, &maxLength, shaderProgramInfoLog);
			MessageBoxA(NULL, shaderProgramInfoLog, NULL, MB_OK);
			free(shaderProgramInfoLog);
		}
		glUseProgram(shaderprogram);
		glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "model_view_mat"), 1, GL_FALSE, &model_view_matrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "proj_mat"), 1, GL_FALSE, &projection_matrix[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas_tex_id);
		glUniform1i(glGetUniformLocation(shaderprogram, "tex"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 6);

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