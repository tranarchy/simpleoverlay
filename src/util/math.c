#include <math.h>
#include <string.h>

#define _MAT4_INDEX(row, column) ((row) + (column) * 4)

void mat4_mult(const float lhs[16], const float rhs[16], float result[16])
{
	int c, r, i;

	for (c = 0; c < 4; c++)
	{
		for (r = 0; r < 4; r++)
		{
			result[_MAT4_INDEX(r, c)] = 0.0f;

			for (i = 0; i < 4; i++)
				result[_MAT4_INDEX(r, c)] += lhs[_MAT4_INDEX(r, i)] * rhs[_MAT4_INDEX(i, c)];
		}
	}
}

void get_projection_matrix(int width, int height, float *projection_matrix)
{
	const float left = 0.0f;
	const float right = (float)width;
	const float bottom = (float)height;
	const float top = 0.0f;
	const float zNear = -1.0f;
	const float zFar = 1.0f;

	const float projection[16] = {
		(2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
		0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
		0.0f, 0.0f, (-2.0f / (zFar - zNear)), 0.0f,

		-((right + left) / (right - left)),
		-((top + bottom) / (top - bottom)),
		-((zFar + zNear) / (zFar - zNear)),
		1.0f,
	};

	memcpy(projection_matrix, projection, 16 * sizeof(float));
}

int lerp(float delta, int start, int end) {
	return start + floor(delta * (float)(end - start));
}
