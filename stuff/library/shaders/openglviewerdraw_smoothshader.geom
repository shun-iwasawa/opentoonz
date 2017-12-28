#version 330 core
 
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;
 
in vec4 vertColor[3];
out vec4 fragColor;
 
void main()
{
  for(int i = 0; i < gl_in.length(); i++)
  {
     // copy attributes
    gl_Position = gl_in[i].gl_Position;
    fragColor = vertColor[i]; 
    // done with the vertex
    EmitVertex();
  }
  //EndPrimitive();
}