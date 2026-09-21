#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#define DIMENSIONS 80
#define REAL_HEIGHT (DIMENSIONS>>1)
#define REAL_WIDTH DIMENSIONS

char BUFFER[REAL_HEIGHT][REAL_WIDTH];

// Source - https://stackoverflow.com/a/67780964
char LIGHT[70] = 
  "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`\'.";

// 2D vector
typedef float vec2[2];
// 3D vector
typedef float vec3[3];
// 2D quadrilateral
typedef vec2 quad2[4];
// 3D quadrilateral
typedef vec3 quad3[4];
// 3D cube
typedef quad3 cube[6];

// Get the magnitude of a 2D vector
float mag2(const vec2 v) {
  return sqrtf((v[0]*v[0])+(v[1]*v[1]));
}

// Get the magnitude of a 3D vector
float mag3(const vec3 v) {
  return sqrtf((v[0]*v[0])+(v[1]*v[1])+(v[2]*v[2]));
}

// Get the inner product of a 2D vector
float dot2(const vec2 a, const vec2 b) {
  return (a[0]*b[0])+(a[1]*b[1]);
}

// Get the inner product of a 3D vector
float dot3(const vec3 a, const vec3 b) {
  return (a[0]*b[0])+(a[1]*b[1])+(a[2]*b[2]);
}

// Normalize a 3D vector
void norm3(vec3 v) {
  float m = mag3(v);
  v[0] /= m;
  v[1] /= m;
  v[2] /= m;
}

// Get the cross product of two vectors and write it into a 3rd
void cross(const vec3 a, const vec3 b, vec3 c) {
  c[0] = a[1]*b[2]-a[2]*b[1];
  c[1] = a[2]*b[0]-a[0]*b[2];
  c[2] = a[0]*b[1]-a[1]*b[0];
}

// Calculate the area of a 2D triangle given 3 points in space
float tri_area(const vec2 a, const vec2 b, const vec2 c) {
  // Get the vector A->B
  vec2 v1 = {a[0]-b[0], a[1]-b[1]};
  // Get the vector perpendicular to the vector C->B
  vec2 v2 = {b[1]-c[1], c[0]-b[0]};
  // Get the magnitude of A->B
  float d = mag2(v1);
  // Get the cosine of the angle lambda between A->B and perp C->B
  float cl = dot2(v1, v2) / (d*mag2(v2));
  // Get the height from the base to point A by using SOHCAHTOA [cos l = h/d]
  float h = d * cl;
  // Get the magnitude of vector C->B as the length of the base
  vec2 v3 = {c[0]-b[0], c[1]-b[1]};
  float bl = mag2(v3);
  // Return (1/2)*base*height as the final area of the triangle
  return fabsf(0.5f*bl*h);
}

// Get the area of a 2D quadrilateral
float quad_area(const quad2 q) {
  // Split the quad into two triangles and get the sum of there areas
  return tri_area(q[0], q[1], q[2]) + tri_area(q[2], q[3], q[0]);
}

// Determine if a 2D point lies within the bounds of a 2D quadrilateral
int point_in_quad(const quad2 q, const vec2 p) {
  // Get the total area of each triangle formed by the point in the quad
  float total_area =  tri_area(q[0], q[1], p) +
                      tri_area(q[1], q[2], p) +
                      tri_area(q[2], q[3], p) +
                      tri_area(q[3], q[0], p);
  // Return the comparison of that area with calculated area of the quad
  return (fabsf(total_area - quad_area(q)) < 0.01f);
}

// Project a 3D quadrilateral to a 2D quadrilateral on the screen
// x: x, y:y, z:depth
void quad3_to_quad2(const quad3 q3, quad2 q2, const float f, 
                    const float camera_depth) {
  // z' = z-camera_depth
  // x' = fx(z^-1)
  // y' = fy(z^-1)
  for (unsigned i = 0; i < 4; ++i) {
    // POTENTIAL FOR DIV-BY-ZERO ERROR HERE
    q2[i][0] = (q3[i][0]*f)/(q3[i][2]-camera_depth);
    q2[i][1] = (q3[i][1]*f)/(q3[i][2]-camera_depth);
  }
}

// Render a 2D quadrilateral to the screen buffer
void render_quad2(const quad2 q, float scale, char c) {
  // Go through each cell of the buffer
  for (int y = 0; y < REAL_HEIGHT; ++y) {
    for (int x = 0; x < REAL_WIDTH; ++x) {
      // Convert screen coordinates to space coordinates 
      vec2 p;
      p[0] = (float)(x - (REAL_HEIGHT))/scale;
      p[1] = (float)((y<<1) - (REAL_HEIGHT))/scale;

      // If the current cell lays above the given quadrilateral, then place a
      // character in the cell
      if (point_in_quad(q, p))
        BUFFER[y][x] = c;
    }
  }
}

// Render a 3D quadrilateral to the screen buffer
void render_quad3(const quad3 q, float scale, 
                  const vec3 lightdir /*assumed normalized*/) {
  // Calculate the normalized vector orthogonal to the quadrilateral
  vec3 a = {q[1][0]-q[0][0], q[1][1]-q[0][1], q[1][2]-q[0][2]};
  vec3 b = {q[2][0]-q[0][0], q[2][1]-q[0][1], q[2][2]-q[0][2]};
  vec3 facing;
  cross(a, b, facing);
  norm3(facing);

  // If the quadrilateral is perpendicular to the direction which the camera is
  // facing then there is no need to render it
  static vec3 camera_facing = {0, 0, 1};
  if (fabsf(dot3(facing, camera_facing)) < 0.45f) return;

  // Determine the light level of the quad based on the direction its facing
  float ln = fabsf(-dot3(facing, lightdir));
  char l = LIGHT[(unsigned)(ln*69)];

  // Convert the 3D quadrilateral to its 2D counterpart
  quad2 qo;
  quad3_to_quad2(q, qo, 1, -4);

  // Finally render the quadrilateral
  render_quad2(qo, scale, l);
}

// Print the buffer to the screen
void draw_buffer(void) {
  for (int y = 0; y < REAL_HEIGHT; ++y) {
    for (int x = 0; x < REAL_WIDTH; ++x) {
      putc(BUFFER[y][x], stdout);
    }
    putc('\n', stdout);
  }
  fflush(stdout);
}

// Clear the buffer by filling it exclusively with spaces
void clear_buffer(void) {
  memset(BUFFER, ' ', REAL_WIDTH*REAL_HEIGHT);
}

// Clear the console screen
void clear_screen(void) {
  printf("\x1b[H");
}

// Rotate a cube around the y-axis by some angle theta
void rotate_around_y(const float theta, const cube q, cube o) {
  for (unsigned j = 0; j < 6; ++j) {
    for (unsigned i = 0; i < 4; ++i) {
      // Simplified matrix multiplication
      o[j][i][0] = q[j][i][0]*cosf(theta)+q[j][i][2]*sinf(theta);
      o[j][i][1] = q[j][i][1];
      o[j][i][2] = -q[j][i][0]*sinf(theta)+q[j][i][2]*cosf(theta);
    }
  }
}

// Rotate a cube around the x-axis by some angle theta
void rotate_around_x(const float theta, const cube q, cube o) {
  for (unsigned j = 0; j < 6; ++j) {
    for (unsigned i = 0; i < 4; ++i) {
      // Simplified matrix multiplication
      o[j][i][0] = q[j][i][0];
      o[j][i][1] = q[j][i][1]*cosf(theta)-q[j][i][2]*sinf(theta);
      o[j][i][2] = q[j][i][1]*sinf(theta)+q[j][i][2]*cosf(theta);
    }
  }
}

// Determine if a quadrilateral contains a given edge vertex
int contains_edge_vertex(const quad3 q, const vec3 v) {
  for (unsigned i = 0; i < 4; ++i) {
    if (q[i][0] == v[0] && q[i][1] == v[1] && q[i][2] == v[2]) return 1;
  }
  return 0;
}

// Render a cube to the screen buffer
void render_cube(const cube c, float scale, const vec3 lightdir) {
  /*
   * Since only up-to three faces of a cube are visible at one time to a single
   * camera, I can simulate back-face culling by determining which corner vertex
   * is closest to the camera and rendering only the faces attached to said
   * vertex.
   */

  // Get the vertex closest to the camera
  vec3 closest_vertex;
  memcpy(closest_vertex, c[0][0], 3*sizeof(float));
  for (unsigned j = 0; j < 6; ++j) {
    for (unsigned i = 0; i < 4; ++i) {
      // Whichever vertex has the smallest depth value is closest to the camera
      if (c[j][i][2] < closest_vertex[2])
        memcpy(closest_vertex, c[j][i], 3*sizeof(float));
    }
  }

  // Render only faces attached to the closest vertex
  for (unsigned j = 0; j < 6; ++j) {
    if (contains_edge_vertex(c[j], closest_vertex))
      render_quad3(c[j], scale, lightdir);
  }
}

int main(int argc, char const *argv[]) {
  printf("\x1b[2J");  // clear screen

  // Define a starting cube
  cube ca = {
    {{-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}},
    {{-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1}},
    {{ 1, -1, -1}, { 1,  1, -1}, { 1,  1,  1}, { 1, -1,  1}},
    {{-1, -1, -1}, {-1,  1, -1}, {-1,  1,  1}, {-1, -1,  1}},
    {{-1,  1, -1}, { 1,  1, -1}, { 1,  1,  1}, {-1,  1,  1}},
    {{-1, -1, -1}, { 1, -1, -1}, { 1, -1,  1}, {-1, -1,  1}}
  };

  // + 2 extra for copying rotation data
  cube cb, cc;

  // Lighting is diagonal
  vec3 lightdir = {1,1,1};
  norm3(lightdir);


  // angle on x and y axes
  float ay = 0;
  float ax = 0;
  
  for (;;) {
    clear_buffer();
    clear_screen();

    rotate_around_y(ay, ca, cb);
    rotate_around_x(ax, cb, cc);
    ay += 0.05f;
    ax += 0.025f;

    render_cube(cc, 80, lightdir);
    draw_buffer();

    usleep(10000);
  }

  return 0;
}