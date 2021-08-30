/*
 * Head rotation matrix: interchange of head orientation data
 * This class doesn't need JUCE!
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

class HeadMatrix
{
public:
    HeadMatrix() : matrixChanged(false)
    {
        eyeMatrix(matrix);
        eyeMatrix(&matrix[9]);
        matRead = matrix;
        matWriteIndex = 9;
        matWrite = &matrix[9];
    }

    // --------------------------------------------------------------------

    void zero()
    {
        eyeMatrix(matWrite);
        commitMatrix();
    }

    // --------------------------------------------------------------------

    bool hasMatrixChanged()
    {
        if (matrixChanged)
        {
            matrixChanged = false;
            return true;
        }
        return false;
    }

    // --------------------------------------------------------------------
        
    void setOrientationYPR(float yawRadian, float pitchRadian, float rollRadian)
    {
        eyeMatrix(matWrite);
        rotatePlaneTranspose(0, 1, yawRadian);
        rotatePlaneTranspose(1, 2, pitchRadian);
        rotatePlaneTranspose(2, 0, rollRadian);
        commitMatrix();
    }

    // --------------------------------------------------------------------

    void setOrientationQuaternion(float w, float x, float y, float z)
    {
        matWrite[0] = w * w + x * x - y * y - z * z;
        matWrite[1] = 2 * (x * y - w * z);
        matWrite[2] = 2 * (x * z + w * y);
        matWrite[3] = 2 * (x * y + w * z);
        matWrite[4] = w * w - x * x + y * y - z * z;
        matWrite[5] = 2 * (y * z - w * x);
        matWrite[6] = 2 * (x * z - w * y);
        matWrite[7] = 2 * (y * z + w * x);
        matWrite[8] = w * w - x * x - y * y + z * z;
        commitMatrix();
    }

    // --------------------------------------------------------------------

    void setOrientationMatrix(const float* mat)
    {
        for (uint8_t i = 0; i < 9; ++i)
        {
            matWrite[i] = mat[i];
        }
        commitMatrix();
    }

    // --------------------------------------------------------------------

    /** Transform body coordinates to world-based coordinates: most
        usefully, to paint the animated head. */
    void transform(float& x, float& y, float &z) const
    {
        // used to paint head
        const float tx = x;
        const float ty = y;
        const float tz = z;
        x = matRead[0] * tx + matRead[1] * ty + matRead[2] * tz;
        y = matRead[3] * tx + matRead[4] * ty + matRead[5] * tz;
        z = matRead[6] * tx + matRead[7] * ty + matRead[8] * tz;
    }

    // --------------------------------------------------------------------
        
    /** Transform world-based coordinates to body coordinates: most
        usefully, to rotate virtual loudspeakers from a room-based to an
        egocentric coordinate system. */
    void transformTranspose(float& x, float& y, float &z) const
    {
        const float tx = x;
        const float ty = y;
        const float tz = z;
        x = matRead[0] * tx + matRead[3] * ty + matRead[6] * tz;
        y = matRead[1] * tx + matRead[4] * ty + matRead[7] * tz;
        z = matRead[2] * tx + matRead[5] * ty + matRead[8] * tz;
    }

    // --------------------------------------------------------------------

    /** Cosines of left- and right-ear poles to the room coordinate [0,-1,0]
        (directed towards the back wall). So returns [0,0] when the listener
        is looking straight ahead, and [1,-1] or [-1,1] when the listener
        has their head turned 90 degrees left or right.
        Useful for certain reverberation models. */
    void getEarVectors(float& left, float& right) const
    {
        // as [0,-1,0] and the rotation matrix entry are both unit vectors,
        // the cosine rule simplifies to cos c = 1 - (C^2 / 2)
        float x = matRead[0];
        float y = matRead[1]+1;
        float z = matRead[2];
        float x2z2 = x*x + z*z;
        right = 1.0f - (x2z2 + y*y)/2.0f;
        y = matRead[1]-1;
        left = 1.0f - (x2z2 + y*y)/2.0f;
    }

    // --------------------------------------------------------------------

private:
    float matrix[2*9];
    float* matWrite;
    float* matRead;
    uint8_t matWriteIndex;
    bool matrixChanged;

    // ------------------------------------------------------------------------

    void eyeMatrix(float* mat)
    {
        // identity matrix
        for (uint8_t i = 0; i < 9; ++i)
        {
            mat[i] = (i & 3) ? 0.f : 1.f;
        }
    }

    // --------------------------------------------------------------------

    void commitMatrix()
    {
        // swap read and write buffers
        matRead = &matrix[matWriteIndex];
        matWriteIndex = 9 - matWriteIndex;
        matWrite = &matrix[matWriteIndex];
        matrixChanged = true;
    }

    // --------------------------------------------------------------------

    void rotatePair(float& ccw, float& cw, float sinAngle, float cosAngle)
    {
        float temp = cosAngle * cw - sinAngle * ccw;
        ccw        = sinAngle * cw + cosAngle * ccw;
        cw = temp;
    }

    // --------------------------------------------------------------------

    void rotatePlane(uint8_t ccwRowIndex, uint8_t cwRowIndex, float angleRadian)
    {
        float sinAngle = sinf(angleRadian);
        float cosAngle = cosf(angleRadian);
        rotatePair(matWrite[ccwRowIndex],   matWrite[cwRowIndex],   sinAngle, cosAngle);
        rotatePair(matWrite[ccwRowIndex+1], matWrite[cwRowIndex+1], sinAngle, cosAngle);
        rotatePair(matWrite[ccwRowIndex+2], matWrite[cwRowIndex+2], sinAngle, cosAngle);
    }

    // --------------------------------------------------------------------

    void rotatePlaneTranspose(uint8_t ccwColIndex, uint8_t cwColIndex, float angleRadian)
    {
        float sinAngle = sinf(angleRadian);
        float cosAngle = cosf(angleRadian);
        rotatePair(matWrite[ccwColIndex],   matWrite[cwColIndex],   sinAngle, cosAngle);
        rotatePair(matWrite[ccwColIndex+3], matWrite[cwColIndex+3], sinAngle, cosAngle);
        rotatePair(matWrite[ccwColIndex+6], matWrite[cwColIndex+6], sinAngle, cosAngle);
    }
};
