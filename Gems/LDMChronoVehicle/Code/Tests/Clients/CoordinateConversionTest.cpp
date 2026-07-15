
#include <AzTest/AzTest.h>

#include <Simulation/CoordinateConversion.h>

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <chrono_vehicle/ChWorldFrame.h>

#include <cmath>

namespace LDMChronoVehicle
{
    namespace
    {
        constexpr float FloatTolerance = 1e-5f;

        void ExpectNear(const AZ::Vector3& actual, const AZ::Vector3& expected)
        {
            EXPECT_NEAR(actual.GetX(), expected.GetX(), FloatTolerance);
            EXPECT_NEAR(actual.GetY(), expected.GetY(), FloatTolerance);
            EXPECT_NEAR(actual.GetZ(), expected.GetZ(), FloatTolerance);
        }

        void ExpectNear(const chrono::ChVector3d& actual, const chrono::ChVector3d& expected)
        {
            EXPECT_NEAR(actual.x(), expected.x(), FloatTolerance);
            EXPECT_NEAR(actual.y(), expected.y(), FloatTolerance);
            EXPECT_NEAR(actual.z(), expected.z(), FloatTolerance);
        }
    } // namespace

    TEST(CoordinateConversionTests, WorldFramesShareZUpConvention)
    {
        // ADR 0002: Chrono stays on its default ISO frame so that both
        // engines agree on a right-handed Z-up world without an axis
        // permutation at the boundary.
        EXPECT_TRUE(chrono::vehicle::ChWorldFrame::IsISO());
        ExpectNear(chrono::vehicle::ChWorldFrame::Vertical(), chrono::ChVector3d(0.0, 0.0, 1.0));
    }

    TEST(CoordinateConversionTests, VectorsMapComponentWiseInBothDirections)
    {
        const AZ::Vector3 azVector(1.5f, -2.25f, 3.75f);
        const chrono::ChVector3d chronoVector = ToChronoVector(azVector);

        ExpectNear(chronoVector, chrono::ChVector3d(1.5, -2.25, 3.75));
        ExpectNear(ToAzVector(chronoVector), azVector);
    }

    TEST(CoordinateConversionTests, QuaternionsAgreeOnRotationInBothEngines)
    {
        // 90 degrees about +Z must take +X to +Y in both math libraries.
        const AZ::Quaternion azRotation =
            AZ::Quaternion::CreateRotationZ(AZ::Constants::HalfPi);
        const chrono::ChQuaterniond chronoRotation = ToChronoQuaternion(azRotation);

        const AZ::Vector3 azRotated = azRotation.TransformVector(AZ::Vector3::CreateAxisX());
        const chrono::ChVector3d chronoRotated =
            chronoRotation.Rotate(chrono::ChVector3d(1.0, 0.0, 0.0));

        ExpectNear(azRotated, AZ::Vector3(0.0f, 1.0f, 0.0f));
        ExpectNear(chronoRotated, chrono::ChVector3d(0.0, 1.0, 0.0));
        ExpectNear(ToAzVector(chronoRotated), azRotated);
    }

    TEST(CoordinateConversionTests, QuaternionRoundTripPreservesRotation)
    {
        const AZ::Quaternion original =
            (AZ::Quaternion::CreateRotationX(0.3f) *
             AZ::Quaternion::CreateRotationY(-1.1f) *
             AZ::Quaternion::CreateRotationZ(2.4f)).GetNormalized();

        const AZ::Quaternion roundTripped = ToAzQuaternion(ToChronoQuaternion(original));

        // q and -q encode the same rotation; compare through a rotated vector.
        const AZ::Vector3 probe(0.6f, -0.7f, 0.4f);
        ExpectNear(roundTripped.TransformVector(probe), original.TransformVector(probe));
    }

    TEST(CoordinateConversionTests, PoseRoundTripPreservesRigidTransform)
    {
        const AZ::Transform original = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationZ(0.75f) * AZ::Quaternion::CreateRotationX(-0.25f),
            AZ::Vector3(10.0f, -20.5f, 4.25f));

        const chrono::ChCoordsysd chronoPose = ToChronoPose(original);
        const AZ::Transform roundTripped = ToAzPose(chronoPose);

        ExpectNear(chronoPose.pos, chrono::ChVector3d(10.0, -20.5, 4.25));
        ExpectNear(roundTripped.GetTranslation(), original.GetTranslation());

        const AZ::Vector3 probe(1.0f, 2.0f, -3.0f);
        ExpectNear(roundTripped.TransformPoint(probe), original.TransformPoint(probe));
        EXPECT_NEAR(roundTripped.GetUniformScale(), 1.0f, FloatTolerance);
    }

    TEST(CoordinateConversionTests, PoseMapsChronoHeightToAzZ)
    {
        // A body 2 m above the ground in Chrono must appear at Z = 2 in O3DE.
        const chrono::ChCoordsysd chronoPose(
            chrono::ChVector3d(0.0, 0.0, 2.0), chrono::ChQuaterniond(1.0, 0.0, 0.0, 0.0));

        const AZ::Transform azPose = ToAzPose(chronoPose);

        EXPECT_NEAR(
            chrono::vehicle::ChWorldFrame::Height(chronoPose.pos),
            static_cast<double>(azPose.GetTranslation().GetZ()),
            FloatTolerance);
    }
} // namespace LDMChronoVehicle
