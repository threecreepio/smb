#version 330
uniform int keys;
uniform int scrollx;
uniform sampler2D tex0;
in vec2 uv;
out vec4 fragment;
void main()
{
    vec2 pos = floor(vec2(uv.y * 0x10, floor(scrollx / 0x10) + (uv.x * 0x10)));
    float tile = texture(tex0, vec2(1.0 / 0x10, 1.0 / 0x200) * pos).r;
    fragment = vec4(tile);
}
