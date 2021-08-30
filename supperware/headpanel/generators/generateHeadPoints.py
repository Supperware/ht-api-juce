# Cross-sections for the wireframe head.
#
# Based on images adapted from Burkhard and Sachs; deployed in my PhD thesis;
# scaled and digitised in Gimp; finished in Microsoft Excel.
#
# Copyright (c) 2021 Supperware Ltd.

xz = [
    [ 0,  119 ],
    [ 11, 118 ],
    [ 20, 116 ],
    [ 30, 112 ],
    [ 43, 104 ],
    [ 53, 91  ],
    [ 58, 83  ],
    [ 62, 73  ],
    [ 65, 54  ],
    [ 66, 38  ],
    [ 65, 30  ],
    [ 69, 32  ],
    [ 72, 29  ],
    [ 73, 25  ],
    [ 74, 15  ],
    [ 71, 0   ],
    [ 65, -13 ],
    [ 60, -19 ],
    [ 58, -19 ],
    [ 56, -17 ],
    [ 50, -30 ],
    [ 43, -44 ],
    [ 38, -50 ],
    [ 33, -53 ]
];

# zero-pad
for i in range (0, len(xz)):
    xz[i] = [ xz[i][0], 0, xz[i][1] ]
#

# -----------------------------------------------------------------------------

xy = [
    [ 0,  90  ],
    [ 4,  87  ],
    [ 8,  81  ],
    [ 9,  79  ],
    [ 15, 74  ],
    [ 29, 68  ],
    [ 41, 62  ],
    [ 53, 54  ],
    [ 60, 42  ],
    [ 64, 30  ],
    [ 65, 24  ],
    [ 66, 9   ],
    [ 74, -11 ],
    [ 73, -18 ],
    [ 67, -18 ],
    [ 65, -24 ],
    [ 61, -34 ],
    [ 55, -44 ],
    [ 48, -53 ],
    [ 38, -61 ],
    [ 26, -68 ],
    [ 14, -72 ],
    [ 0,  -74 ]
];

# zero-pad
for i in range (0, len(xy)):
    xy[i] = [ xy[i][0], xy[i][1], 0 ]
#

# -----------------------------------------------------------------------------

yz = [
    [ -50, -76 ],
    [ -52, -68 ],
    [ -53, -63 ],
    [ -56, -47 ],
    [ -61, -35 ],
    [ -65, -24 ],
    [ -70, -12 ],
    [ -77, 7   ],
    [ -81, 28  ],
    [ -81, 41  ],
    [ -80, 57  ],
    [ -74, 74  ],
    [ -68, 84  ],
    [ -57, 99  ],
    [ -40, 111 ],
    [ -20, 119 ],
    [ -0,  119 ],
    [ 15,  118 ],
    [ 30,  114 ],
    [ 49,  106 ],
    [ 58,  99  ],
    [ 70,  84  ],
    [ 74,  75  ],
    [ 79,  56  ],
    [ 80,  47  ],
    [ 80,  37  ],
    [ 79,  29  ],
    [ 81,  23  ],
    [ 90,  0   ],
    [ 92,  -10 ],
    [ 90,  -13 ],
    [ 77,  -17 ],
    [ 75,  -18 ],
    [ 76,  -20 ],
    [ 81,  -26 ],
    [ 80,  -28 ],
    [ 68,  -33 ],
    [ 79,  -36 ],
    [ 80,  -38 ],
    [ 77,  -41 ],
    [ 75,  -46 ],
    [ 74,  -53 ],
    [ 75,  -58 ],
    [ 75,  -62 ],
    [ 72,  -65 ],
    [ 68,  -68 ],
    [ 62,  -70 ],
    [ 58,  -71 ],
    [ 50,  -69 ],
    [ 38,  -65 ],
    [ 27,  -62 ],
    [ 20,  -59 ],
    [ 14,  -53 ]
];

# zero-pad
for i in range (0, len(yz)):
    yz[i] = [ 0, yz[i][0], yz[i][1] ]
#

# -----------------------------------------------------------------------------

hz = [
    [ -48, -77 ],
    [ -48, -68 ],
    [ -51, -47 ],
    [ -55, -33 ],
    [ -63, -12 ],
    [ -71, 7   ],
    [ -75, 28  ],
    [ -75, 41  ],
    [ -74, 57  ],
    [ -71, 74  ],
    [ -66, 84  ],
    [ -55, 99  ],
    [ -38, 111 ],
    [ -17, 118 ],
    [ 0,   119 ],
    [ 20,  117 ],
    [ 40,  108 ],
    [ 51,  99  ],
    [ 61,  87  ],
    [ 67,  75  ],
    [ 71,  58  ],
    [ 72,  47  ],
    [ 72,  37  ],
    [ 73,  29  ],
    [ 75,  20  ],
    [ 75,  5   ],
    [ 72,  -10 ],
    [ 68,  -19 ],
    [ 64,  -31 ],
    [ 59,  -42 ],
    [ 50,  -54 ],
    [ 45,  -58 ],
    [ 39,  -61 ],
    [ 35,  -61 ],
];

# zero-pad
oneoverroot2 = 0.5**0.5
for i in range (0, len(hz)):
    hz[i] = [ hz[i][0] * oneoverroot2, hz[i][0] * oneoverroot2, hz[i][1] ]
#

# -----------------------------------------------------------------------------

neckring = [
    [ -50, 0,   -82 ],
    [ -45, -18, -79 ],
    [ -34, -34, -77 ],
    [ -18, -45, -76 ],
    [ 0,   -50, -76 ],
    [ 18,  -45, -76 ],
    [ 34,  -34, -77 ],
    [ 45,  -18, -79 ],
    [ 50,  0,   -82 ],
    [ 47,  17,  -85 ],
    [ 35,  35,  -89 ],
    [ 18,  44,  -92 ],
    [ 0,   48,  -93 ],
    [ -18, 44,  -92 ],
    [ -35, 35,  -89 ],
    [ -47, 17,  -85 ]
];

# -----------------------------------------------------------------------------

''' bounds[0] is the maximum absolute value of all x, y, or z coordinates
    processed by this script. bounds[1] is the maximum resultant, or norm:
    call it what you will. '''
def addToBounds(points):
    global bounds
    for i in range(0, len(points)):
        p = points[i]
        extent = (p[0]*p[0] + p[1]*p[1] + p[2]*p[2]) ** 0.5
        #
        m = max(abs(p[0]), abs(p[1]), abs(p[2]))
        if (m > bounds[0]):
            bounds[0] = m
        #
        if (extent > bounds[1]):
            bounds[1] = extent
        #
    #
#

def scale(points, divisor):
    for i in range(0, len(points)):
        points[i] = [ points[i][0] / divisor, points[i][1] / divisor, points[i][2] / divisor];
    #
    return points
#

def getAsString(points, name):
    global nl
    s = nl;
    s += '    static constexpr int ' + name + 'Count = ' + str(len(points)) + ';' + nl
    s += '    static constexpr V3 ' + name + '[' + name + 'Count] {' + nl
    for i in range(0, len(points)):
        if (i == len(points)-1):
            comma = ''
        else:
            comma = ','
        #
        n = '        { %.3ff, %.3ff, %.3ff }%s' % (points[i][0], points[i][1], points[i][2], comma)
        s += n + nl
    #
    s += '    };' + nl
    return s
#

# -----------------------------------------------------------------------------

# bounds[0] : projected max;  bounds[1] : absolute max
bounds = [0,0]
addToBounds(xz)
addToBounds(xy)
addToBounds(yz)
addToBounds(hz)
addToBounds(neckring)

print(bounds)

xz = scale(xz, bounds[1])
xy = scale(xy, bounds[1])
yz = scale(yz, bounds[1])
hz = scale(hz, bounds[1])
neckring = scale(neckring, bounds[1])

nl = '\r\n'
f = open('headpanel-Points.h','wb')
s =  '''/*
 * Head tracker panel and driver API
 * This file was autogenerated by generateHeadPoints.py
 * Copyright (c) 2021 Supperware Ltd.
 */''' + nl + nl
s += '#pragma once' + nl
s += nl + 'namespace HeadPanel' + nl
s += '{'
 
s += nl + '    typedef float V3[3];' + nl

s += getAsString(xz, 'XZ')
s += getAsString(yz, 'YZ')
s += getAsString(xy, 'XY')
s += getAsString(hz, 'HZ')
s += getAsString(neckring, 'NeckRing')
s += '};' + nl

# it'd be nice to write s directly; unfortunately that messes up the line endings.
f.write(bytearray(s.encode()))
f.close()
