#version 330
uniform int keys;
uniform int scrollx;
uniform sampler2D tex0;
in vec2 uv;
out vec4 fragment;
void main()
{
    float b16 = 1.0 / 0x10;
    float stagelen = 0x200;

    vec2 basepos = vec2(uv.y, uv.x + (scrollx / float(0x10) * b16));
    vec2 pos = floor(vec2(basepos.x * 0x10, basepos.y * 0x10));
    float tile = texture(tex0, vec2(b16, 1.0 / stagelen) * pos).r;
    fragment = vec4(tile);
}
