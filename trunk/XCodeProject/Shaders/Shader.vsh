//
//  Shader.vsh
//  FingerMania
//
//  Created by 李 杰军 on 10-12-28.
//  Copyright 2010 Virtuos. All rights reserved.
//

attribute vec4 position;
attribute vec4 color;

varying vec4 colorVarying;

uniform float translate;

void main()
{
    gl_Position = position;
    gl_Position.y += sin(translate) / 2.0;

    colorVarying = color;
}
