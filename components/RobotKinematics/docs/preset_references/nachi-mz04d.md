This is Nachi robot MZ04D's preset reference for Kinematics setting.
Pair of joint point and coordinate get from robot teach pedant, not setting tool (tool offset 0, 0, 0, 0 ,0, 0), robot original frame coordination.    
Joint:
    1  J(28.1579,-18.8069,163.839,-0.710019,35.8922,152.731)
    2  J(46.3996,-18.8029,163.841,-0.710019,35.8898,152.727)
    3  J(-2.49169,-18.8029,163.843,-0.710019,35.8898,152.727)
    4  J(28.1557,-13.0437,163.841,-0.710019,35.8898,152.727)
    5  J(28.1557,-30.531,163.843,-0.710019,35.8898,152.727)
    6  J(28.1557,-18.8049,180.206,-0.710019,35.8898,152.727)
    7  J(28.1557,-18.8029,147.018,-0.710019,35.8898,152.727)
    8  J(28.1557,-18.8049,163.841,34.0763,35.8898,152.727)
    9  J(28.1557,-18.8029,163.841,-35.5607,35.8898,152.727)
    10  J(28.1557,-18.8049,163.841,-0.714615,69.0539,152.727)
    11  J(28.1557,-18.7989,163.841,-0.714615,7.70156,152.721)
    12  J(28.1557,-18.8029,163.841,-0.710019,35.8898,200.78)
    13  J(28.1557,-18.8029,163.841,-0.712317,35.8898,120.206)
    14  J(28.2282,-18.9278,164.21,0.00919119,34.718,151.759)
    15  J(31.122,-16.8986,160.187,21.2041,48.5086,133.088)
    16  J(29.6938,-20.1833,169.043,25.1747,18.7945,127.285)
    17  J(28.2282,-18.9238,164.212,0.0137868,34.718,219.293)
    18  J(32.1613,-18.6659,165.74,40.5814,34.3617,128.805)
    19  J(23.2932,-15.8386,159.494,-35.6112,53.2523,185.217)
    20  J(-0.00219726,0.00430813,179.996,0.00459559,0.0046875,-0.000121055)
    21  J(0,90.002,-0.00217014,0.00459559,-0.00234375,6.05273e-05)

Coordinate of tool flange (center of wrist coordinate system): X, Y, Z, RZ, RY, RX
    1  X(339.15,182.108,571.95,0.301758,-1.00829,0.060508)
    2  X(265.078,279.092,571.996,18.5415,-1.01493,0.0655774)
    3  X(384.555,-16.1959,572.015,-30.3416,-1.01655,0.0663919)
    4  X(316.881,170.172,609.464,0.129697,-6.10352,2.76823)
    5  X(373.609,200.531,488.92,-0.10146,9.3413,-5.45622)
    6  X(255.728,137.446,598.296,-0.835413,-15.4284,7.91414)
    7  X(414.47,222.397,518.315,-0.604066,13.8207,-7.93948)
    8  X(355.761,163.565,567.837,29.5244,3.54526,19.2647)
    9  X(333.466,206.294,567.493,-26.5584,-14.1285,-14.8798)
    10  X(304.422,163.866,559.622,-4.14049,-30.099,16.8315)
    11  X(369.405,197.824,564.041,-2.70127,23.8103,-13.6682)
    12  X(339.126,182.077,571.996,48.3522,-0.72722,-0.711033)
    13  X(339.117,182.074,572.001,-32.2176,-0.821677,0.600015)
    14  X(339.088,182.008,572.065,-0.00376925,-0.00358024,0.00786421)
    15  X(339.088,181.928,572.142,-0.0238987,-0.00493311,18.4547)
    16  X(339.057,182.004,572.09,-0.00233472,16.0384,0.0113963)
    17  X(339.062,182.009,572.097,67.5266,-0.0106346,-0.000248291)
    18  X(339.082,181.968,572.085,17.0802,11.0767,19.3541)
    19  X(339.119,182.571,571.895,6.5187,-18.922,-23.4265)
    20  X(234.998,-0.00901241,692.029,-179.995,0.00212387,-2.86984e-08)
    21  X(351.991,4.50987e-07,624.996,-61.782,-89.9947,-118.218)

Both poses 20, 21 when most of robot axis joint's at 0 degree.
Pose 21 is initialize pose for reference robot link in official manual from Nachi.

DH parameters (user guess):
        theta       d       alpha       a
    J1  0           157.5   0           0
    J2  180.0       182.5   -90.0       0
    J3  0           260.0   0           0
    J4  90.0        0.0     90.0        25.0
    J5  -180.0      280.0   180.0       0
    J5  180.0       72.0    180.0       0
Maybe the orent or length when make the parameters table was misunderstand, be minus or plus.

Joint limits (from robot teach pendant):
J1 (-170.0, 170.0)
J2 (-145.0, 90.0)
J3 (-125.0, 280.0)
J4 (-190.0, 190.0)
J5 (-120.0, 120.0)
J6 (-360.0, 360.0)

---

## Reverse-engineering results (2026-06-19)

The guessed DH table above does not reproduce the measurements (position errors ~500 mm).
Two corrections were required:

1. **Orientation convention.** The pendant reports the orientation triple **Z-first**:
   `(yaw about Z, pitch about Y, roll about X)`, i.e. the rotation is
   `R = Rz(1st)·Ry(2nd)·Rx(3rd)` of the three reported orientation numbers.
   (Interpreting the first number as roll-X makes orientation unfittable while position
   still fits — that was the original confusion.) When building an expected `Pose`, map the
   first number to yaw and the third to roll.

2. **Length placement.** The provided lengths are correct, but several sit in the wrong DH
   column/row. The shoulder height is `340 mm = 157.5 + 182.5` (the J1 and J2 axes intersect
   340 mm above the base), and the upper-arm `260` and elbow offset `25` are `a` (link)
   values, not `d`.

### Corrected standard DH (`Rz(theta) Tz(d) Tx(a) Rx(alpha)`)

|     | theta | d (mm) | alpha (deg) | a (mm) |
|-----|-------|--------|-------------|--------|
| J1  | 0     | 340.0  | +90         | 0      |
| J2  | 0     | 0      | 0           | 260    |
| J3  | 0     | 0      | +90         | 25     |
| J4  | 0     | 280    | -90         | 0      |
| J5  | 0     | 0      | +90         | 0      |
| J6  | 0     | 72     | 0           | 0      |

This reproduces all 21 reference poses to **<= 0.035 mm and <= 0.010 deg**. It is encoded as
canonical joint origins in `src/Presets/NachiMZ04D.cpp` / `presets/Nachi/MZ04/nachi_mz04d.json` and
verified point-by-point in `tests/integration/NachiMZ04DTests.cpp`. Joint limits above are
applied to the preset.

For IK tests that compare an expected joint vector against a solution, use the project default
joint-angle comparison tolerance of **<= 0.0001 degree** per revolute joint. If a fixture derives
its expected pose from teach-pendant coordinates rounded to 2 decimal places, that specific
joint-vector comparison may relax to **<= 0.001 degree** and should document the coordinate
precision next to the assertion. This exception is for joint-vector comparison only; it does not
relax TCP pose residual checks.

**Posture (Arm config) is mapped from the Nachi manual rules below:** shoulder = sign(J1)
(J1<0 righty, J1>0 lefty), wrist = sign(J5) (J5<0 flip, J5>0 non-flip), elbow = sign(J3)
(J3<0 above, J3>0 below). The Arm-config ground-truth set (4 configs, all below/non-flip,
2 lefty + 2 righty) confirms shoulder/wrist and the elbow below-side (J3>0); the elbow
above-side (J3<0) is inferred by symmetry. Encoded in the preset `posture.labels` and verified
in `tests/integration/NachiMZ04DTests.cpp`.

The Arm-config ground-truth set also doubles as a **tool-offset FK check**: with tool offset
`(44.2, 0, 139.0) mm` the 4 TCP poses are reproduced to <= 0.04 mm / 0.005 deg
(`forwardKinematicsWithToolMatchesMeasuredPoses`).

Important example note: `examples/Robot3DVizualize/Centering_tool.stl` uses a different
example-local visual/tool offset of `(45, 0, 112, 0, 0, 0)`. Do not replace this measured FK
fixture value with the STL/example value unless the underlying preset test data is re-measured.

## Arm config

**This config condition from Nachi Official manual**

###  RIGHTY and LEFTY

Righty: when J1 < 0.0 deg
Lefty: when J1 > 0.0 deg

### ABOVE and BELOW

Above: when On elbow 
Below: when Under elbow

**User not sure On elbow and below elbow meaning**

### FLIP and NON-FLIP

Flip: when J5 < 0.0
Non-Flip: when J5 > 0.0

## Arm config test data

Joint:
    1  J(25.5519,-22.283,174.08,-0.767464,29.1492,155.426)
    2  J(23.124,-18.8593,164.056,-0.569853,35.8219,157.649)
    3  J(-26.3452,-22.0674,173.422,1.03631,29.5313,205.746)
    4  J(-23.4909,-17.9565,161.556,0.77206,37.3102,203.18)

Coordinate with tool (center of offset: 44.2, 0, 139.0, 0, 0, 0): X, Y, Z, RZ, RY, RX
    1  X(357.026,151.287,711.747,0.302115,-1.01447,0.066286)
    2  X(394.634,151.222,711.924,0.307335,-1.0669,0.0869993)
    3  X(356.991,-156.748,711.81,0.302113,-1.01847,0.0591564)
    4  X(401.18,-156.814,711.822,0.307374,-1.02259,0.0604472)

1, 2, 3, 4 are non-flip
1, 2, 3, 4 are below
1, 2 are lefty
3, 4 are righty
