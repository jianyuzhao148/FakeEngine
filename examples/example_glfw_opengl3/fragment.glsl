#version 330 core
out vec4 FragColor;
in vec3 ourColor;
in vec3 FragPos;
void main()
{
    if(abs(FragPos.x)>0.49 && abs(FragPos.z) > 0.49||abs(FragPos.x)>0.49 && abs(FragPos.y) > 0.49
    ||(FragPos.y)>0.49 && (FragPos.z) > 0.49){
       FragColor =vec4(1.0,0.0,1.0,1.0f);
    }else{
       FragColor =vec4(0.0,1.0,1.0,1.0f);
    }
}
