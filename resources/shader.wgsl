// {Begin block 'file: resources/shader.wgsl' (in root '037 - Loading from file - Next')}
// In a new file 'resources/shader.wgsl'
// Move the content of the global `shaderSource` variable (and remove that variable from main.cpp)
// {Begin block 'Shader source' (in root '033 - Multiple Attributes - Option A - Next')}
// {Begin block 'Shader prelude' (in root '039 - A first uniform - Next')}
// {Begin block 'Define VertexInput struct' (in root '033 - Multiple Attributes - Option A - Next')}
/**
 * A structure with fields labeled with vertex attribute locations can be used
 * as input to the entry point of a shader.
 */
struct VertexInput {
	@location(0) position: vec2f,
	@location(1) color: vec3f,
};
// {End block 'Define VertexInput struct' (in root '033 - Multiple Attributes - Option A - Next')}
// {Begin block 'Define VertexOutput struct' (in root '033 - Multiple Attributes - Option A - Next')}
/**
 * A structure with fields labeled with builtins and locations can also be used
 * as *output* of the vertex shader, which is also the input of the fragment
 * shader.
 */
struct VertexOutput {
	@builtin(position) position: vec4f,
	// The location here does not refer to a vertex attribute, it just means
	// that this field must be handled by the rasterizer.
	// (It can also refer to another field of another struct that would be used
	// as input to the fragment shader.)
	@location(0) color: vec3f,
};
// {End block 'Define VertexOutput struct' (in root '033 - Multiple Attributes - Option A - Next')}
// We add the declaration of 'uTime' to the shader prelude
// {Begin block 'Declare uniforms' (in root '039 - A first uniform - Next')}
// The memory location of the uniform is given by a pair of a *bind group* and a *binding*
@group(0) @binding(0)
var<uniform> uTime: f32;
// {End block 'Declare uniforms' (in root '039 - A first uniform - Next')}
// {End block 'Shader prelude' (in root '039 - A first uniform - Next')}

@vertex
// {Begin block 'Vertex shader' (in root '039 - A first uniform - Next')}
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let ratio = 640.0 / 480.0;

	// We now move the scene depending on the time!
	var offset = vec2f(-0.6875, -0.463);
	offset += 0.3 * vec2f(cos(uTime), sin(uTime));

	out.position = vec4f(in.position.x + offset.x, (in.position.y + offset.y) * ratio, 0.0, 1.0);
	out.color = in.color;
	return out;
}
// {End block 'Vertex shader' (in root '039 - A first uniform - Next')}

@fragment
// {Begin block 'Fragment shader' (in root '033 - Multiple Attributes - Option A - Next')}
// Or we can use a custom struct whose fields are labeled
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	//     ^^^^^^^^^^^^^^^^ Use for instance the same struct as what the vertex outputs
	// {Begin block 'Fragment shader body' (in root '033 - Multiple Attributes - Option A - Next')}
	// In fs_main()
	return vec4f(in.color, 1.0); // use the interpolated color coming from the vertex shader
	// {End block 'Fragment shader body' (in root '033 - Multiple Attributes - Option A - Next')}
}
// {End block 'Fragment shader' (in root '033 - Multiple Attributes - Option A - Next')}
// {End block 'Shader source' (in root '033 - Multiple Attributes - Option A - Next')}
// {End block 'file: resources/shader.wgsl' (in root '037 - Loading from file - Next')}