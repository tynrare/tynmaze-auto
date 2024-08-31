#version 100

precision lowp float;

varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;
varying mat3 TBN;

uniform sampler2D texture0;
uniform sampler2D tex_noise0;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float rand(vec3 co) {
    return fract(sin(dot(co.xyz, vec3(12.9898, 78.233, 138.1203))) * 43758.5453);
}

void main()
{
    vec2 tiles = vec2(5.0, 3.0);
    vec2 rev_tiles = 1.0 / tiles;
	
    bool top = fragNormal.y < 0.0;
    bool bottom = fragNormal.y > 0.0;
    bool side_a = fragNormal.x > 0.0 || fragNormal.z > 0.0;
    bool side_b = fragNormal.x < 0.0 || fragNormal.z < 0.0;

    vec2 uv = fragTexCoord;
		uv *= rev_tiles;

		// discard uvs to zero
    if (top) {
        uv.y -= rev_tiles.y; 
    } else if(bottom) {
        uv.x -= rev_tiles.x; // discard to zero
    } else if(side_b || side_a) {
        if (side_b) {
            uv.x -= rev_tiles.x; // discard to zero
        }
    }

		float pad = 0.0;
		uv *= 1.0 - pad;
		uv += rev_tiles * pad * 0.5;

		// restore uvs to new texture positions
    if (top) {
    } else if(bottom) {
        uv.y += rev_tiles.y * 2.0;
    } else if(side_b || side_a) {
        uv.y += rev_tiles.y;
    }

    float gridscale = 1.0;
		// 0.01 required for fixing glitching related with ceils hopping
		// (kinda z-fight)
    vec3 fp = vec3(fragPosition.x + 0.501, fragPosition.y, fragPosition.z + 0.501) * gridscale;
    vec3 fp_ceil = ceil(fp) / gridscale;

    // completely random texture
    //float randomValue = rand(fp_ceil);
    // texture based value
		vec2 ruv = abs(fract(fp_ceil.xy * 0.01));
		vec4 randomValues = texture2D(tex_noise0, ruv);
		float randomValue = randomValues.x;

    uv.x += ceil(randomValue * tiles.x) * rev_tiles.x;

    vec4 color = texture2D(texture0, fract(uv));

    gl_FragColor = vec4(color.rgb, 1.0);
}

