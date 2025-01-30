// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "catch.h"
#include "Utils/MathFuncs.h"
#include "Utils/Matrix4.h"

static bool matrix_equals(const matrix4 &a, const matrix4 &b) {
    return a.equals(b, 0.00001f);
}

static v3f x{1, 0, 0};
static v3f y{0, 1, 0};
static v3f z{0, 0, 1};

TEST_CASE("matrix4") {

SECTION("setRotationRadians") {
    SECTION("rotation order is ZYX (matrix notation)") {
        v3f rot{1, 2, 3};
        matrix4 X, Y, Z, ZYX;
        X.setRotationRadians({rot.X, 0, 0});
        Y.setRotationRadians({0, rot.Y, 0});
        Z.setRotationRadians({0, 0, rot.Z});
        ZYX.setRotationRadians(rot);
        CHECK(!matrix_equals(X * Y * Z, ZYX));
        CHECK(!matrix_equals(X * Z * Y, ZYX));
        CHECK(!matrix_equals(Y * X * Z, ZYX));
        CHECK(!matrix_equals(Y * Z * X, ZYX));
        CHECK(!matrix_equals(Z * X * Y, ZYX));
        CHECK(matrix_equals(Z * Y * X, ZYX));
    }

    const f32 quarter_turn = PI / 2;

    // See https://en.wikipedia.org/wiki/Right-hand_rule#/media/File:Cartesian_coordinate_system_handedness.svg
    // for a visualization of what handedness means for rotations

    SECTION("rotation is right-handed") {
        SECTION("rotation around the X-axis is Z-up, counter-clockwise") {
            matrix4 X;
            X.setRotationRadians({quarter_turn, 0, 0});
            CHECK(X.transformVect(x) == x);
            CHECK(X.transformVect(y) == z);
            CHECK(X.transformVect(z) == -y);
        }

        SECTION("rotation around the Y-axis is Z-up, clockwise") {
            matrix4 Y;
            Y.setRotationRadians({0, quarter_turn, 0});
            CHECK(Y.transformVect(y) == y);
            CHECK(Y.transformVect(x) == -z);
            CHECK(Y.transformVect(z) == x);
        }

        SECTION("rotation around the Z-axis is Y-up, counter-clockwise") {
            matrix4 Z;
            Z.setRotationRadians({0, 0, quarter_turn});
            CHECK(Z.transformVect(z) == z);
            CHECK(Z.transformVect(x) == y);
            CHECK(Z.transformVect(y) == -x);
        }
    }
}

}
