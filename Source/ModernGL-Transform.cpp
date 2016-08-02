#include "ModernGL.hpp"

#include "OpenGL.hpp"
#include "BufferFormat.hpp"

PyObject * NewTransformArray(PyObject * self, PyObject * args, PyObject * kwargs) {
	Program * program;
	VertexBuffer * src;
	VertexBuffer * dst;

	const char * format;
	PyObject * attributes;

	if (!PyArg_ParseTuple(args, "O!O!sO!:NewTransformArray", &ProgramType, &program, &VertexBufferType, &src, &VertexBufferType, &dst, &format, &PyList_Type, &attributes)) {
		return 0;
	}

	int count = (int)PyList_Size(attributes);

	FormatIterator it = FormatIterator(format);
	FormatInfo info = it.info();

	if (!info.valid) {
		PyErr_Format(ModuleInvalidFormat, "NewTransformArray() format is invalid");
		return 0;
	}

	if (info.nodes != count) {
		PyErr_Format(ModuleInvalidFormat, "NewTransformArray() format has %d nodes but there are %d attributes", info.nodes, count);
		return 0;
	}

	for (int i = 0; i < count; ++i) {
		const char * name = PyUnicode_AsUTF8(PyList_GET_ITEM(attributes, i));
		int location = OpenGL::glGetAttribLocation(program->program, name);
		if (strict && location < 0) {
			PyErr_Format(ModuleAttributeNotFound, "NewVertexArray() attribute `%s` not found", name);
			return 0;
		}
	}

	int tao = 0;
	OpenGL::glGenVertexArrays(1, (OpenGL::GLuint *)&tao);
	OpenGL::glBindVertexArray(tao);

	OpenGL::glBindBuffer(OpenGL::GL_ARRAY_BUFFER, src->vbo);

	int i = 0;
	char * ptr = 0;
	while (FormatNode * node = it.next()) {
		const char * name = PyUnicode_AsUTF8(PyList_GET_ITEM(attributes, i++));
		int location = OpenGL::glGetAttribLocation(program->program, name);
		OpenGL::glVertexAttribPointer(location, node->count, node->type, false, info.size, ptr);
		OpenGL::glEnableVertexAttribArray(location);
		ptr += node->count * node->size;
	}

	OpenGL::glBindVertexArray(defaultVertexArray);
	return CreateTransformArrayType(program->program, tao, src->vbo, dst->vbo);
}

PyObject * DeleteTransformArray(PyObject * self, PyObject * args) {
	VertexArray * tao;

	if (!PyArg_ParseTuple(args, "O!:DeleteTransformArray", &TransformArrayType, &tao)) {
		return 0;
	}

	OpenGL::glDeleteVertexArrays(1, (OpenGL::GLuint *)&tao->tao);
	Py_RETURN_NONE;
}

PyObject * NewTransformShader(PyObject * self, PyObject * args) {
	const char * source;

	if (!PyArg_ParseTuple(args, "s:NewTransformShader", &source)) {
		return 0;
	}

	int shader = OpenGL::glCreateShader(OpenGL::GL_VERTEX_SHADER);
	OpenGL::glShaderSource(shader, 1, &source, 0);
	OpenGL::glCompileShader(shader);

	int compiled = OpenGL::GL_FALSE;
	OpenGL::glGetShaderiv(shader, OpenGL::GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		static const char * logTitle = "NewTransformShader() compile failed\n";
		static int logTitleLength = strlen(logTitle);
		
		int logLength = 0;
		OpenGL::glGetShaderiv(shader, OpenGL::GL_INFO_LOG_LENGTH, &logLength);
		int logTotalLength = logLength + logTitleLength;

		PyObject * content = PyUnicode_New(logTotalLength, 255);
		if (PyUnicode_READY(content)) {
			OpenGL::glDeleteShader(shader);
			return 0;
		}

		char * data = (char *)PyUnicode_1BYTE_DATA(content);
		memcpy(data, logTitle, logTitleLength);

		int logSize = 0;
		OpenGL::glGetShaderInfoLog(shader, logLength, &logSize, data + logTitleLength);
		data[logTitleLength] = 0;

		OpenGL::glDeleteShader(shader);

		PyErr_SetObject(ModuleCompileError, content);
		return 0;
	}

	int program = OpenGL::glCreateProgram();

	OpenGL::glAttachShader(program, shader);

	int linked = OpenGL::GL_FALSE;
	OpenGL::glLinkProgram(program);
	OpenGL::glGetProgramiv(program, OpenGL::GL_LINK_STATUS, &linked);

	if (!linked) {
		static const char * logTitle = "NewTransformShader() linking failed\n";
		static int logTitleLength = strlen(logTitle);
		
		int logLength = 0;
		OpenGL::glGetProgramiv(program, OpenGL::GL_INFO_LOG_LENGTH, &logLength);
		int logTotalLength = logLength + logTitleLength;

		PyObject * content = PyUnicode_New(logTotalLength, 255);
		if (PyUnicode_READY(content)) {
			OpenGL::glDeleteProgram(program);
			OpenGL::glDeleteShader(shader);
			return 0;
		}

		char * data = (char *)PyUnicode_1BYTE_DATA(content);
		memcpy(data, logTitle, logTitleLength);

		int logSize = 0;
		OpenGL::glGetProgramInfoLog(program, logLength, &logSize, data + logTitleLength);
		data[logTitleLength] = 0;

		OpenGL::glDeleteProgram(program);
		OpenGL::glDeleteShader(shader);

		PyErr_SetObject(ModuleCompileError, content);
		return 0;
	}

	return CreateTransformShaderType(shader, program);
}

PyObject * DeleteTransformShader(PyObject * self, PyObject * args) {
	TransformShader * shader;

	if (!PyArg_ParseTuple(args, "O!:DeleteTransformShader", &TransformShaderType, &shader)) {
		return 0;
	}

	OpenGL::glDeleteProgram(shader->program);
	OpenGL::glDeleteShader(shader->shader);
	Py_RETURN_NONE;
}

PyObject * RunTransformShader(PyObject * self, PyObject * args, PyObject * kwargs) {
	TransformShader * shader;
	TransformArray * tao;
	int vertices;
	int first = 0;

	static const char * kwlist[] = {"shader", "tao", "vertices", "first", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!i|i:RunTransformShader", (char **)kwlist, &TransformShaderType, &shader, &TransformArrayType, &tao, &vertices, &first)) {
		return 0;
	}

	if (activeProgram != tao->program) {
		OpenGL::glUseProgram(tao->program);
		activeProgram = tao->program;
	}

	OpenGL::glBindVertexArray(tao->tao);
	glBindBufferBase(OpenGL::GL_TRANSFORM_FEEDBACK_BUFFER, 0, tao->dst);

	glEnable(GL_RASTERIZER_DISCARD);
	glBeginTransformFeedback(GL_POINTS);
	OpenGL::glDrawArrays(OpenGL::GL_POINTS, first, vertices);
	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);
	glFlush();

	OpenGL::glBindVertexArray(defaultVertexArray);
	Py_RETURN_NONE;
}
