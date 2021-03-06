//  Created by William Talmadge on 6/12/14.
/*
 Copyright (C) 2014  William B. Talmadge
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef __Image_Shader__FFTPermuteColumns__
#define __Image_Shader__FFTPermuteColumns__

#include <iostream>
#include <vector>
#include "ISShaderProgram.h"
#include "ISDrawable.h"
#include "ISVertexArray.h"
#include "ISSingleton.h"
#include <GLKit/GLKMatrix4.h>
#include <string>
#include "assert.h"

struct FFTPermute : public ISDrawable<ISSingleton, ISSingleton, FFTPermute>, ISSingletonBindable {
    static const std::string fragShader;
    static const std::string vertShaderRows;
    static const std::string vertShaderCols;
    
    using ISDrawableT = ISDrawable<ISSingleton, ISSingleton, FFTPermute>;
    enum class Orientation { Rows, Cols }; //Cols indicates that the signal spans across columns, if the orientation is Cols we are transforming rows. Consider changing this as it might be a confusing convention
    enum class Stride { SkipNone, SkipOne };
    enum class Offset { Zero, One };
    GLfloat stride() {
        if (_stride == Stride::SkipNone) {
            return 1.0;
        } else {
            return 2.0;
        }
    }
    GLfloat offset() {
        if (_offset == Offset::Zero) {
            return 0.0;
        } else  {
            return 1.0;
        }
    }
    GLfloat size() {
        if (_orientation == Orientation::Cols) {
            return _targetROI.width();
        } else {
            return _targetROI.height();
        }
    }
    GLfloat length() {
        if (_orientation == Orientation::Rows) {
            return _targetROI.width();
        } else {
            return _targetROI.height();
        }
    }
    //TODO: abstract _orthoMatrix to base
    FFTPermute(Stride stride, Offset offset, Orientation orientation, std::vector<GLuint> butterflyPlan) : ISDrawableT(), _stride(stride), _offset(offset), _orientation(orientation), _butterflyPlan(butterflyPlan) {
    }
    void init() {
        if (_orientation == Orientation::Cols) {
            //FIXME: this can fire if the source data is a real signal with no factors of 2 in it.
            assert(stride()*_targetROI.width() == _sourceROI.width());
            assert(_targetROI.height() == _sourceROI.height());
        } else if (_orientation == Orientation::Rows) {
            assert(stride()*_targetROI.height() == _sourceROI.height());
            assert(_targetROI.width() == _sourceROI.width());
        }
    }
    GLuint textureBindingTarget() const {
        assert(_isSetup);
        return _textureUniformPosition;
    }
    void bindUniforms(ISSingleton* inputTuple, ISSingleton* outputTuple) {
        glUniform1f(_strideUP, stride());
        glUniform1f(_offsetUP, offset());
        if (_orientation == Orientation::Cols) {
            glUniform1f(_sourceSizeUP, _sourceSize.width());
        } else {
            glUniform1f(_sourceSizeUP, _sourceSize.height());
        }
        
    }
    size_t hashImpl() const {
        size_t result;
        if (_orientation == Orientation::Cols) {
            result = std::hash<size_t>()(0);
        } else {
            result = std::hash<size_t>()(1);
        }
        for (size_t b : _butterflyPlan) {
            result ^= b;
        }
        return result;
    }
    bool compareImpl(const FFTPermute& rhs) const {
        bool result = true;
        if (_butterflyPlan.size() != rhs._butterflyPlan.size()) {
            result = false;
        } else {
            result = _orientation == rhs._orientation;
            for (size_t i = 0; i < _butterflyPlan.size(); i++) {
                result &= _butterflyPlan[i] == rhs._butterflyPlan[i];
            }
        }
        return result;
    };
    void drawImpl() { };
    void permute_(std::vector<GLuint>& permutation,
                  GLuint sumForward,
                  GLuint sumReverse,
                  GLuint radixForward, //alpha set
                  GLuint radixReverse, //beta set
                  std::vector<GLuint>::iterator radixFactor,
                  std::vector<GLuint>::iterator end)
    {
        
        if (radixFactor != end)
        {
            std::vector<GLuint>::iterator nextRadixFactor = radixFactor;
            ++nextRadixFactor;
            for (GLuint s = 0; s < *radixFactor; s++)
            {
                
                permute_(permutation,
                         sumForward + s*radixForward,
                         sumReverse + s*(radixReverse/(*radixFactor)),
                         radixForward*(*radixFactor),
                         radixReverse/(*radixFactor),
                         nextRadixFactor,
                         end);
            }
        }
        else
        {
            permutation[sumReverse] = sumForward;
        }
    }
    void setupGeometry() {
        ISVertexArray* geometry = new ISVertexArray();
        //First reverse the butterfly set so we start from R_0 instead of R_M-1
        reverse(_butterflyPlan.begin(), _butterflyPlan.end());
        int n = 1;
        for (GLuint i = 0; i < _butterflyPlan.size();i++) {
            n *= _butterflyPlan[i];
        }
        assert(n == size());
        std::vector<GLuint> permutationTable(size());
        permutationTable.resize(size());
        permute_(permutationTable, 0, 0, 1, size(), _butterflyPlan.begin(), _butterflyPlan.end());
        
        std::vector<GLfloat> vertices;
        vertices.reserve(size()*5*6);
        if (_orientation == Orientation::Cols) {
            float h = (float)length()/_sourceSize.height();
            for (GLuint i = 0; i < size(); i++) {
                GLfloat source = static_cast<GLfloat>(_sourceROI.left() + permutationTable[i]);
                std::vector<GLfloat>
                quad = makeGlAttributePixelColumn(length(), _targetROI.left() + i, _targetROI.left() + i+1,
                                                  {0.0, source},
                                                  {h, source}); //This is a bit confusing but the vert shader actually may swap these depending on the orientation
                vertices.insert(vertices.end(), quad.begin(), quad.end());
            }
        } else if (_orientation == Orientation::Rows) {
            float h = (float)length()/_sourceSize.width();
            for (GLuint i = 0; i < size(); i++) {
                GLfloat source = static_cast<GLfloat>(_sourceROI.top() + permutationTable[i]);
                appendGlLookupRow(vertices, length(), h, _targetROI.top() + i, _targetROI.top() + i+1,
                                  {source}, {source}, {}, {});
            }
        }
        geometry->addFloatAttribute(0, 3);
        geometry->addFloatAttribute(1, 1);
        geometry->addFloatAttribute(2, 1);
        geometry->upload(vertices);
        _geometry = geometry;
    }
    void setupShaderProgram(ISShaderProgram* program) {
        std::vector<ShaderAttribute> attributeMap{
            {0, "positionIn"},
            {1, "texCoordVIn"},
            {2, "sourceIndexIn"}
        };
        if (_orientation == Orientation::Cols) {
            program->loadShader(fragShader, prependOrthoMatrixUniform(vertShaderCols), attributeMap);
        } else {
            program->loadShader(fragShader, prependOrthoMatrixUniform(vertShaderRows), attributeMap);
        }
    }
    void resolveUniformPositions() {
        _textureUniformPosition = glGetUniformLocation(_program->program(), "inputImageTexture");
        _strideUP = glGetUniformLocation(_program->program(), "stride");
        _offsetUP = glGetUniformLocation(_program->program(), "offset");
        _sourceSizeUP = glGetUniformLocation(_program->program(), "sourceSize");
    }
    
protected:
    std::vector<GLuint> _butterflyPlan;
    
    GLuint _textureUniformPosition;
    GLuint _strideUP;
    GLuint _offsetUP;
    GLuint _sourceSizeUP;
    
    Orientation _orientation;
    Stride _stride;
    Offset _offset;
};
#endif /* defined(__Image_Shader__FFTPermuteColumns__) */
