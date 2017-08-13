//////////////////////////////////////////////////////////////////////
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////


float CalculateDiffuseStrength( float3 normal, float3 lightDir )
{
   return saturate( -dot( normal, lightDir ) );
}

float CalculateSpecularStrength( float3 normal, float3 lightDir, float3 eyeDir )
{
   float3 diff    = saturate( dot(normal, -lightDir) );
   float3 reflect = normalize( 2 * diff * normal + lightDir ); 
   
   return saturate( dot( reflect, -eyeDir ) );
}

float CalculateDirectionalLight( float3 normal, float3 lightDir, float3 eyeDir, float specularPow, float specularMul )
{
   float3 light0 = normalize( lightDir );

   return CalculateDiffuseStrength( normal, light0 ) + specularMul * pow( CalculateSpecularStrength( normal, light0, eyeDir ), specularPow );
}
