/*
 * Head tracker panel and driver API
 * Plots a wireframe head in the required orientation
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace HeadPanel
{
    class HeadPlot
    {
    public:
        HeadPlot() :
            pointList(),
            cosGaze(1.0f),
            sinGaze(0.0f)
        {}
        
        // --------------------------------------------------------------------

        /** Rotates head appropriately to a new position */
        void recalculate(const HeadMatrix& headMatrix)
        {
            ScopedLock sl(calculateOrPaint);
            pointList.clear();
            uint8_t i;

            //neck-ring
            for (i = 0; i < NeckRingCount; ++i)
            {
                project3D(NeckRing[i], headMatrix, false, false);
            }
            project3D(NeckRing[0], headMatrix, false, true);

            //xy
            for (i = XYCount - 1; i > 0; --i)
            {
                project3D(XY[i], headMatrix, true, false);
            }
            for (i = 0; i < XYCount; ++i)
            {
                project3D(XY[i], headMatrix, false, i == XYCount - 1); 
            }

            //xz
            for (i = XZCount - 1; i > 0; --i)
            {
                project3D(XZ[i], headMatrix, true, false);
            }
            for (i = 0; i < XZCount; ++i)
            {
                project3D(XZ[i], headMatrix, false, i == XZCount - 1);
            }

            //yz
            for (i = 0; i < YZCount; ++i)
            {
                project3D(YZ[i], headMatrix, false, i == YZCount - 1);
            }

            //hz1
            for (i = 0; i < HZCount; ++i)
            {
                project3D(HZ[i], headMatrix, false, i == HZCount - 1);
            }

            //hz2
            for (i = 0; i < HZCount; ++i)
            {
                project3D(HZ[i], headMatrix, true, i == HZCount - 1);
            }
        }

        // --------------------------------------------------------------------

        void paint(Graphics& g, const float xMid, const float yMid,
            const float scale, const float lineThickness, const Midi::State connectionState) const
        {
            Rectangle<float> bounds(xMid - scale, yMid - scale, 2.0f * scale, 2.0f * scale);

            if (connectionState == Midi::State::Bootloader)
            {
                g.setFont(juce::Font (15.0f));
                g.setColour(Colour(0xff808080));
                g.drawText("BOOTLOADER", bounds, Justification::centred);
            }
            else if (connectionState == Midi::State::Connected)
            {
                ScopedLock sl(calculateOrPaint);
                g.setColour(Colours::black);
                g.fillEllipse(xMid - scale, yMid - (scale * sinGaze), scale * 2.0f, scale * sinGaze * 2.0f);
                pointList.paint(g, static_cast<int>(xMid), static_cast<int>(yMid), scale, lineThickness);
            }
            else
            {
                g.setColour(Colour(0xff282828));
                g.drawEllipse(bounds, lineThickness);
            }
        }

        // --------------------------------------------------------------------

        /** Zero is a rear view; 90 is a plan view; you can have anything in-between */
        void setGazeAngle(const float gazeDegrees)
        {
            // 0 to 90
            float gazeRadians = gazeDegrees * MathConstants<float>::pi / 180.0f;
            sinGaze = sinf(gazeRadians);
            cosGaze = cosf(gazeRadians);
        }

        // --------------------------------------------------------------------

    private:
        static constexpr int Heatmap_Count = 18;
        const Colour DefaultHeatmap[Heatmap_Count] = {
            Colour(0x88580818), Colour(0xcc580818), Colour(0xf0580818),
            Colour(0xff800820), Colour(0xffa01030), Colour(0xffc0183c),
            Colour(0xffd0284a), Colour(0xffd8345c), Colour(0xffe04c6a),
            Colour(0xffec5878), Colour(0xfff46484), Colour(0xfff87890),
            Colour(0xfffc88a0), Colour(0xffff98ac), Colour(0xffffa0bc),
            Colour(0xffffacc8), Colour(0xffffbcd8), Colour(0xffffd0ec)
        };

        // --------------------------------------------------------------------

        PointList pointList;
        float cosGaze, sinGaze;
        CriticalSection calculateOrPaint;

        // --------------------------------------------------------------------
        //                                                        3D PROJECTION
        // --------------------------------------------------------------------

        void project3D(const V3 v, const HeadMatrix& headMatrix,
            const bool flipX, const bool closeLine)
        {
            Vector3D<float> t(flipX ? v[0] : -v[0], v[1], v[2]);
            headMatrix.transform(t.x, t.y, t.z);

            float ty  = cosGaze * t.y - sinGaze * t.z;
            t.z       = cosGaze * t.z + sinGaze * t.y;
            t.y = ty;

            int tempInten = static_cast<int>(8.8f * (1.0f - ty));
            Colour c = (tempInten < 0) ? DefaultHeatmap[0] :
                       (tempInten >= Heatmap_Count) ? DefaultHeatmap[Heatmap_Count - 1] :
                        DefaultHeatmap[tempInten];

            pointList.addPoint(t.x, t.y, t.z, c, closeLine);
        }
    };
};
