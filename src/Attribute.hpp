#pragma once

#include <Python.h>

#include "ProgramMember.hpp"
#include "Program.hpp"

struct MGLAttribute : public MGLProgramMember {
	PyObject * name;

	void * gl_attrib_ptr_proc;

	int number;
	int location;
	int type;

	int scalar_type;

	int dimension;
	int array_length;

	int rows_length;
	int row_length;
	int row_size;

	char shape;
	bool normalizable;
};

extern PyTypeObject MGLAttribute_Type;

MGLAttribute * MGLAttribute_New();
void MGLAttribute_Invalidate(MGLAttribute * attribute);
void MGLAttribute_Complete(MGLAttribute * attribute, const GLMethods & gl);

typedef GLvoid (GLAPI * gl_attribute_normal_ptr_proc)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
typedef GLvoid (GLAPI * gl_attribute_ptr_proc)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
