/*
 * Head tracker panel and driver API
 * Z-buffered linked list for storing points and drawing wireframes
 * Copyright (c) 2021 Supperware Ltd.
 */

#pragma once

namespace HeadPanel
{
    class PointList
    {
    public:
        
        struct PointItem
        {
            PointItem(float _x, float _y, float _z, Colour _colour, bool _closeLine):
                x(_x),
                y(_y),
                z(_z),
                colour(_colour),
                linkForwards(-1),
                linkBackwards(-1),
                closeLine(_closeLine)
            {}

            float x, y, z;
            Colour colour;
            signed int linkForwards, linkBackwards;
            bool closeLine;
        };
        
        // ------------------------------------------------------------------------

        PointList() :
            points(),
            rearItem(-1)
        {
            points.reserve(200); // approximate size of the wireframe head
        }

        // ------------------------------------------------------------------------

        void clear()
        {
            points.clear();
            rearItem = -1;
        }

        // ------------------------------------------------------------------------

        void addPoint(const float x, const float y, float z, const Colour colour, const bool closeLine)
        {
            points.push_back(PointItem(x, y, z, colour, closeLine));
            PointItem* p = &points[points.size() - 1];
            int thisIndex = static_cast<int>(points.size() - 1);

            // insert into linked list, sorted according to y descending
            // (so rear item has the highest y), using the previous point
            // as a starting place
            if (rearItem < 0)
            {
                rearItem = 0;
            }
            else
            {
                int prevIndex, nextIndex;
                findSurroundingPoints(thisIndex, prevIndex, nextIndex);

                p->linkBackwards = prevIndex;
                p->linkForwards = nextIndex;
                if (prevIndex >= 0) { points[prevIndex].linkForwards  = thisIndex; }
                if (nextIndex >= 0) { points[nextIndex].linkBackwards = thisIndex; }
                else { rearItem = thisIndex; }
            }
        }

        // ------------------------------------------------------------------------

        void paint(Graphics& g, const int xMid, const int yMid, const float scale, const float lineThickness) const
        {
            int index = rearItem;
            while (index != -1)
            {
                const PointItem* p1 = &points[index];
                if (!p1->closeLine)
                {
                    // the connecting point is always added to the list in sequence.
                    // if we went backwards through the linked list here, our next point
                    // would not necessarily be related to this line ...
                    const PointItem* p2 = &points[index + 1];
                    g.setColour(p1->colour);
                    g.drawLine(xMid + p1->x * scale, yMid - p1->z * scale,
                               xMid + p2->x * scale, yMid - p2->z * scale, lineThickness);
                }
                index = p1->linkBackwards;
            }
        }

        // ------------------------------------------------------------------------

        void findSurroundingPoints(const int thisIndex, int& prevIndex, int& nextIndex)
        {
            float depth = points[thisIndex].y;

            prevIndex = -1;
            nextIndex = -1;

            // use the previously-placed point as a starting point
            int i = static_cast<int>(points.size()) - 2;
            if (i >= 0)
            {
                if (points[i].y > depth)
                {
                    // traverse backwards
                    while ((i >= 0) && (points[i].y > depth))
                    {
                        nextIndex = i;
                        i = points[i].linkBackwards;
                    }
                    prevIndex = i;
                }
                else
                {
                    // traverse forwards
                    while ((i >= 0) && (points[i].y <= depth))
                    {
                        prevIndex = i;
                        i = points[i].linkForwards;
                    }
                    nextIndex = i;
                }
            }
        }

        // ------------------------------------------------------------------------

    private:
        std::vector<PointItem> points;
        int rearItem;
    };
};
