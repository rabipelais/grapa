#version 330

//varying vec3 N;
//varying vec3 V;
//varying vec3 L;
in vec4 normalC;
out vec4 outputColor;
void main()
{
      
    /* vec3 E = normalize(-V); // we are in Eye Coordinates, so EyePos is (0,0,0)   */
    /* vec3 R = normalize(-reflect(L,N)); */
    
    /* vec4 k_amb = gl_FrontLightProduct[0].ambient; */
    
    /* vec4 k_diff = gl_FrontLightProduct[0].diffuse * max(0.0, dot(L, N)); */
    /* k_diff = clamp(k_diff, 0.0, 1.0); */

    /* vec4 k_spec = gl_FrontLightProduct[0].specular * pow(max(dot(R,E),0.0), 0.3 * gl_FrontMaterial.shininess); */
    /* k_spec = clamp(k_spec, 0.0, 1.0); */

    /* gl_FragColor =  k_amb + k_diff + k_spec; */
    //outputColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    outputColor = normalC;
}
