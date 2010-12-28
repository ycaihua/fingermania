//
//  Shader.fsh
//  FingerMania
//
//  Created by 李 杰军 on 10-12-28.
//  Copyright 2010 Virtuos. All rights reserved.
//

varying lowp vec4 colorVarying;

void main()
{
    gl_FragColor = colorVarying;
}
