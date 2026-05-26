// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#define CMS_NO_REGISTER_KEYWORD 1
#include <lcms2.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct Matrix3
{
    double m[3][3];
};

enum class CurveType
{
    Linear,
    Gamma,
    SRGB
};

struct Profile
{
    std::string filename;
    std::string description;
    cmsCIExyY white;
    cmsCIExyYTRIPLE primaries;
    CurveType curveType;
    double gamma;
};

static cmsCIExyY
xyY(double x, double y)
{
    cmsCIExyY value;
    value.x = x;
    value.y = y;
    value.Y = 1.0;
    return value;
}

static cmsCIEXYZ
xyYToXYZ(const cmsCIExyY& value)
{
    cmsCIEXYZ xyz;
    xyz.X = value.x / value.y;
    xyz.Y = 1.0;
    xyz.Z = (1.0 - value.x - value.y) / value.y;
    return xyz;
}

static cmsCIExyYTRIPLE
primaries(double rx, double ry, double gx, double gy, double bx, double by)
{
    cmsCIExyYTRIPLE result;
    result.Red = xyY(rx, ry);
    result.Green = xyY(gx, gy);
    result.Blue = xyY(bx, by);
    return result;
}

static Matrix3
multiply(const Matrix3& a, const Matrix3& b)
{
    Matrix3 r = {};

    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            for (int k = 0; k < 3; ++k)
                r.m[y][x] += a.m[y][k] * b.m[k][x];

    return r;
}

static cmsCIEXYZ
multiply(const Matrix3& a, const cmsCIEXYZ& v)
{
    cmsCIEXYZ r;
    r.X = a.m[0][0] * v.X + a.m[0][1] * v.Y + a.m[0][2] * v.Z;
    r.Y = a.m[1][0] * v.X + a.m[1][1] * v.Y + a.m[1][2] * v.Z;
    r.Z = a.m[2][0] * v.X + a.m[2][1] * v.Y + a.m[2][2] * v.Z;
    return r;
}

static Matrix3
inverse(const Matrix3& a)
{
    const double det =
        a.m[0][0] * (a.m[1][1] * a.m[2][2] - a.m[1][2] * a.m[2][1]) -
        a.m[0][1] * (a.m[1][0] * a.m[2][2] - a.m[1][2] * a.m[2][0]) +
        a.m[0][2] * (a.m[1][0] * a.m[2][1] - a.m[1][1] * a.m[2][0]);

    Matrix3 r = {};

    r.m[0][0] =  (a.m[1][1] * a.m[2][2] - a.m[1][2] * a.m[2][1]) / det;
    r.m[0][1] = -(a.m[0][1] * a.m[2][2] - a.m[0][2] * a.m[2][1]) / det;
    r.m[0][2] =  (a.m[0][1] * a.m[1][2] - a.m[0][2] * a.m[1][1]) / det;

    r.m[1][0] = -(a.m[1][0] * a.m[2][2] - a.m[1][2] * a.m[2][0]) / det;
    r.m[1][1] =  (a.m[0][0] * a.m[2][2] - a.m[0][2] * a.m[2][0]) / det;
    r.m[1][2] = -(a.m[0][0] * a.m[1][2] - a.m[0][2] * a.m[1][0]) / det;

    r.m[2][0] =  (a.m[1][0] * a.m[2][1] - a.m[1][1] * a.m[2][0]) / det;
    r.m[2][1] = -(a.m[0][0] * a.m[2][1] - a.m[0][1] * a.m[2][0]) / det;
    r.m[2][2] =  (a.m[0][0] * a.m[1][1] - a.m[0][1] * a.m[1][0]) / det;

    return r;
}

static Matrix3
rgbToXYZMatrix(const cmsCIExyY& white, const cmsCIExyYTRIPLE& prim)
{
    const cmsCIEXYZ r = xyYToXYZ(prim.Red);
    const cmsCIEXYZ g = xyYToXYZ(prim.Green);
    const cmsCIEXYZ b = xyYToXYZ(prim.Blue);
    const cmsCIEXYZ w = xyYToXYZ(white);

    Matrix3 p = {};
    p.m[0][0] = r.X; p.m[0][1] = g.X; p.m[0][2] = b.X;
    p.m[1][0] = r.Y; p.m[1][1] = g.Y; p.m[1][2] = b.Y;
    p.m[2][0] = r.Z; p.m[2][1] = g.Z; p.m[2][2] = b.Z;

    const cmsCIEXYZ s = multiply(inverse(p), w);

    Matrix3 result = {};
    result.m[0][0] = r.X * s.X;
    result.m[0][1] = g.X * s.Y;
    result.m[0][2] = b.X * s.Z;
    result.m[1][0] = r.Y * s.X;
    result.m[1][1] = g.Y * s.Y;
    result.m[1][2] = b.Y * s.Z;
    result.m[2][0] = r.Z * s.X;
    result.m[2][1] = g.Z * s.Y;
    result.m[2][2] = b.Z * s.Z;

    return result;
}

static Matrix3
bradfordAdaptationMatrix(const cmsCIEXYZ& sourceWhite, const cmsCIEXYZ& targetWhite)
{
    Matrix3 bradford = {};
    bradford.m[0][0] =  0.8951; bradford.m[0][1] =  0.2664; bradford.m[0][2] = -0.1614;
    bradford.m[1][0] = -0.7502; bradford.m[1][1] =  1.7135; bradford.m[1][2] =  0.0367;
    bradford.m[2][0] =  0.0389; bradford.m[2][1] = -0.0685; bradford.m[2][2] =  1.0296;

    const Matrix3 bradfordInv = inverse(bradford);
    const cmsCIEXYZ sourceCone = multiply(bradford, sourceWhite);
    const cmsCIEXYZ targetCone = multiply(bradford, targetWhite);

    Matrix3 scale = {};
    scale.m[0][0] = targetCone.X / sourceCone.X;
    scale.m[1][1] = targetCone.Y / sourceCone.Y;
    scale.m[2][2] = targetCone.Z / sourceCone.Z;

    return multiply(bradfordInv, multiply(scale, bradford));
}

static cmsCIEXYZ
columnToXYZ(const Matrix3& matrix, int column)
{
    cmsCIEXYZ value;
    value.X = matrix.m[0][column];
    value.Y = matrix.m[1][column];
    value.Z = matrix.m[2][column];
    return value;
}

static void
writeUInt32BE(std::vector<unsigned char>& data, cmsUInt32Number value)
{
    data.push_back(static_cast<unsigned char>((value >> 24) & 0xff));
    data.push_back(static_cast<unsigned char>((value >> 16) & 0xff));
    data.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    data.push_back(static_cast<unsigned char>(value & 0xff));
}

static cmsS15Fixed16Number
toS15Fixed16(double value)
{
    return static_cast<cmsS15Fixed16Number>(std::round(value * 65536.0));
}

static bool
writeChromaticAdaptationTag(cmsHPROFILE handle, const Matrix3& matrix)
{
    std::vector<unsigned char> data;

    writeUInt32BE(data, cmsSigS15Fixed16ArrayType);
    writeUInt32BE(data, 0);

    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            writeUInt32BE(data, static_cast<cmsUInt32Number>(toS15Fixed16(matrix.m[y][x])));

    return cmsWriteRawTag(
        handle,
        cmsSigChromaticAdaptationTag,
        data.data(),
        static_cast<cmsUInt32Number>(data.size())
    ) != 0;
}

static cmsToneCurve*
buildToneCurve(const Profile& profile)
{
    switch (profile.curveType) {
        case CurveType::Linear:
            return cmsBuildGamma(nullptr, 1.0);

        case CurveType::Gamma:
            return cmsBuildGamma(nullptr, profile.gamma);

        case CurveType::SRGB: {
            // ICC parametric curve type 4:
            // Y = (aX + b)^g for X >= d
            // Y = cX          for X <  d
            //
            // sRGB / Display P3 decoding curve:
            // linear RGB = ((encoded + 0.055) / 1.055)^2.4
            // linear RGB = encoded / 12.92
            cmsFloat64Number params[5] = {
                2.4,
                1.0 / 1.055,
                0.055 / 1.055,
                1.0 / 12.92,
                0.04045
            };

            return cmsBuildParametricToneCurve(nullptr, 4, params);
        }
    }

    return nullptr;
}

static bool
writeProfile(const Profile& profile)
{
    const cmsCIExyY d50xy = xyY(0.34567, 0.35850);
    const cmsCIEXYZ d50 = xyYToXYZ(d50xy);
    const cmsCIEXYZ sourceWhite = xyYToXYZ(profile.white);

    const Matrix3 rgbToXYZ = rgbToXYZMatrix(profile.white, profile.primaries);
    const Matrix3 chad = bradfordAdaptationMatrix(sourceWhite, d50);
    const Matrix3 adaptedRgbToXYZ = multiply(chad, rgbToXYZ);

    cmsToneCurve* curve = buildToneCurve(profile);

    if (!curve) {
        std::fprintf(stderr, "Failed to create tone curve: %s\n", profile.description.c_str());
        return false;
    }

    cmsToneCurve* curves[3] = {
        curve,
        curve,
        curve
    };

    cmsHPROFILE handle = cmsCreateRGBProfile(
        &d50xy,
        &profile.primaries,
        curves
    );

    if (!handle) {
        cmsFreeToneCurve(curve);
        std::fprintf(stderr, "Failed to create profile: %s\n", profile.description.c_str());
        return false;
    }

    cmsSetProfileVersion(handle, 4.3);
    cmsSetHeaderRenderingIntent(handle, INTENT_PERCEPTUAL);

    cmsCIEXYZ red = columnToXYZ(adaptedRgbToXYZ, 0);
    cmsCIEXYZ green = columnToXYZ(adaptedRgbToXYZ, 1);
    cmsCIEXYZ blue = columnToXYZ(adaptedRgbToXYZ, 2);

    cmsWriteTag(handle, cmsSigMediaWhitePointTag, &d50);
    cmsWriteTag(handle, cmsSigRedColorantTag, &red);
    cmsWriteTag(handle, cmsSigGreenColorantTag, &green);
    cmsWriteTag(handle, cmsSigBlueColorantTag, &blue);
    writeChromaticAdaptationTag(handle, chad);

    cmsMLU* description = cmsMLUalloc(nullptr, 1);
    cmsMLU* copyright = cmsMLUalloc(nullptr, 1);

    cmsMLUsetASCII(description, "en", "US", profile.description.c_str());
    cmsMLUsetASCII(
        copyright,
        "en",
        "US",
        "Copyright 2022-present Contributors to the colorpicker project."
    );

    cmsWriteTag(handle, cmsSigProfileDescriptionTag, description);
    cmsWriteTag(handle, cmsSigCopyrightTag, copyright);

    const bool saved = cmsSaveProfileToFile(handle, profile.filename.c_str()) != 0;

    cmsMLUfree(description);
    cmsMLUfree(copyright);
    cmsCloseProfile(handle);
    cmsFreeToneCurve(curve);

    if (!saved) {
        std::fprintf(stderr, "Failed to save profile: %s\n", profile.filename.c_str());
        return false;
    }

    std::printf("Generated: %s\n", profile.filename.c_str());
    return true;
}

// main
int
main(int argc, const char* argv[])
{
    const cmsCIExyY d65 = xyY(0.3127, 0.3290);
    const cmsCIExyY d60 = xyY(0.32168, 0.33767);
    const cmsCIExyY dci = xyY(0.3140, 0.3510);

    std::vector<Profile> profiles = {
        {
            "ACESCG Linear.icc",
            "ACEScg Linear",
            d60,
            primaries(0.713, 0.293, 0.165, 0.830, 0.128, 0.044),
            CurveType::Linear,
            1.0
        },
        {
            "AdobeRGB1998.icc",
            "Adobe RGB (1998)",
            d65,
            primaries(0.640, 0.330, 0.210, 0.710, 0.150, 0.060),
            CurveType::Gamma,
            563.0 / 256.0
        },
        {
            "DCI(P3) RGB.icc",
            "DCI-P3 RGB Gamma 2.6",
            dci,
            primaries(0.680, 0.320, 0.265, 0.690, 0.150, 0.060),
            CurveType::Gamma,
            2.6
        },
        {
            "Display P3.icc",
            "Display P3",
            d65,
            primaries(0.680, 0.320, 0.265, 0.690, 0.150, 0.060),
            CurveType::SRGB,
            2.4
        },
        {
            "ITU-709 Gamma 2.2.icc",
            "ITU-R BT.709 Gamma 2.2",
            d65,
            primaries(0.640, 0.330, 0.300, 0.600, 0.150, 0.060),
            CurveType::Gamma,
            2.2
        },
        {
            "ITU-709 Gamma 2.4.icc",
            "ITU-R BT.709 Gamma 2.4",
            d65,
            primaries(0.640, 0.330, 0.300, 0.600, 0.150, 0.060),
            CurveType::Gamma,
            2.4
        },
        {
            "ITU-2020 Gamma 2.2.icc",
            "ITU-R BT.2020 Gamma 2.2",
            d65,
            primaries(0.708, 0.292, 0.170, 0.797, 0.131, 0.046),
            CurveType::Gamma,
            2.2
        },
        {
            "ITU-2020 Gamma 2.4.icc",
            "ITU-R BT.2020 Gamma 2.4",
            d65,
            primaries(0.708, 0.292, 0.170, 0.797, 0.131, 0.046),
            CurveType::Gamma,
            2.4
        },
        {
            "sRGB Profile.icc",
            "sRGB Profile",
            d65,
            primaries(0.640, 0.330, 0.300, 0.600, 0.150, 0.060),
            CurveType::SRGB,
            2.4
        }
    };

    bool ok = true;

    for (const Profile& profile : profiles) {
        ok = writeProfile(profile) && ok;
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
