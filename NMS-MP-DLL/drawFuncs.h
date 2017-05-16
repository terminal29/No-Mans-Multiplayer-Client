#pragma once
#include <GL/gl3w.h>
#include <GL/GL.h>
#include <GL/GLU.h>

void drawCube(GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat sideLength) {
	glUseProgram(0);

	GLchar* vertexsource = "#version 150	\
		in  vec3 in_Position;						\
		//in  vec3 in_Color;							\
		out vec3 ex_Color;							\
		void main(void) {							\
			gl_Position = gl_ModelViewProjectionMatrix * vec4(in_Position.x, in_Position.y, in_Position.z, 1.0);	\
			ex_Color = vec4(1.0,1.0,1.0,1.0);					\
		}";

	GLchar* fragmentsource = "#version 150	\
			precision highp float;					\
													\
		in  vec3 ex_Color;							\
		out vec4 gl_FragColor;						\
													\
		void main(void) {							\
			gl_FragColor = vec4(ex_Color, 1.0);		\
		}";

	/* We're going to create a simple diamond made from lines */
	GLfloat halfSideLength = sideLength * 0.5;

	static const GLfloat g_vertex_buffer_data[] = {
		-1.0f,-1.0f,-1.0f, // triangle 1 : begin
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, // triangle 1 : end
		1.0f, 1.0f,-1.0f, // triangle 2 : begin
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f, // triangle 2 : end
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f
	};


	int i; /* Simple iterator */
	GLuint vao, vbo; /* Create handles for our Vertex Array Object and two Vertex Buffer Objects */
						/* These pointers will receive the contents of our shader source code files */

						/* These are handles used to reference the shaders */
	GLuint vertexshader, fragmentshader;

	/* This is a handle to the shader program */
	GLuint shaderprogram;

	/* Allocate and assign a Vertex Array Object to our handle */
	glGenVertexArrays(1, &vao);

	/* Bind our Vertex Array Object as the current used object */
	glBindVertexArray(vao);

	/* Allocate and assign two Vertex Buffer Objects to our handle */
	glGenBuffers(1, &vbo);

	/* Bind our first VBO as being the active buffer and storing vertex attributes (coordinates) */
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	/* Copy the vertex data from diamond to our buffer */
	/* 8 * sizeof(GLfloat) is the size of the diamond array, since it contains 8 GLfloat values */
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), g_vertex_buffer_data, GL_STATIC_DRAW);

	/* Specify that our coordinate data is going into attribute index 0, and contains two floats per vertex */
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	/* Enable attribute index 0 as being used */
	glEnableVertexAttribArray(0);

	/* Bind our second VBO as being the active buffer and storing vertex attributes (colors) */
	//glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

	/* Copy the color data from colors to our buffer */
	/* 12 * sizeof(GLfloat) is the size of the colors array, since it contains 12 GLfloat values */
	//glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), colors, GL_STATIC_DRAW);

	/* Specify that our color data is going into attribute index 1, and contains three floats per vertex */
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	/* Enable attribute index 1 as being used */
	//glEnableVertexAttribArray(1);

	/* Create an empty vertex shader handle */
	vertexshader = glCreateShader(GL_VERTEX_SHADER);

	/* Send the vertex shader source code to GL */
	/* Note that the source code is NULL character terminated. */
	/* GL will automatically detect that therefore the length info can be 0 in this case (the last parameter) */
	glShaderSource(vertexshader, 1, (const GLchar**)&vertexsource, 0);

	/* Compile the vertex shader */
	glCompileShader(vertexshader);



	/* Create an empty fragment shader handle */
	fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);

	/* Send the fragment shader source code to GL */
	/* Note that the source code is NULL character terminated. */
	/* GL will automatically detect that therefore the length info can be 0 in this case (the last parameter) */
	glShaderSource(fragmentshader, 1, (const GLchar**)&fragmentsource, 0);

	/* Compile the fragment shader */
	glCompileShader(fragmentshader);

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
	//glBindAttribLocation(shaderprogram, 1, "in_Color");

	/* Link our program */
	/* At this stage, the vertex and fragment programs are inspected, optimized and a binary code is generated for the shader. */
	/* The binary code is uploaded to the GPU, if there is no error. */
	glLinkProgram(shaderprogram);

	/* Load the shader into the rendering pipeline */
	glUseProgram(shaderprogram);

	/* Loop our display increasing the number of shown vertexes each time.
	* Start with 2 vertexes (a line) and increase to 3 (a triangle) and 4 (a diamond) */
	/* Invoke glDrawArrays telling that our data is a line loop and we want to draw 2-4 vertexes */
	glDrawArrays(GL_TRIANGLES, 0, 12*3);

	/* Cleanup all the things we bound and allocated */
	glUseProgram(0);
	glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
	glDetachShader(shaderprogram, vertexshader);
	glDetachShader(shaderprogram, fragmentshader);
	glDeleteProgram(shaderprogram);
	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}