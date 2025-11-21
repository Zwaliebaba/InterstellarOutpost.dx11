cbuffer SceneConstants : register(b0)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 gInvView;
    float4x4 gInvProj;
    float3 gCameraPos;
    float pad0;
    float3 gLightDir;
    float pad1;
    float2 gViewportSize;
    float2 pad2;
};

struct Voxel
{
    float3 center;
    float3 halfSize;
    float4 rotation; // quaternion
    float4 color;
};

StructuredBuffer<Voxel> gVoxels : register(t0);

struct VSIn
{
    float2 pos : POS;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    uint voxelIndex : VOXEL;
};

float3x3 QuaternionToMatrix(float4 q)
{
    float x2 = q.x + q.x;
    float y2 = q.y + q.y;
    float z2 = q.z + q.z;

    float xx2 = q.x * x2;
    float yy2 = q.y * y2;
    float zz2 = q.z * z2;
    float xy2 = q.x * y2;
    float xz2 = q.x * z2;
    float yz2 = q.y * z2;
    float wx2 = q.w * x2;
    float wy2 = q.w * y2;
    float wz2 = q.w * z2;

    float3x3 m;
    m[0] = float3(1.0 - (yy2 + zz2), xy2 + wz2, xz2 - wy2);
    m[1] = float3(xy2 - wz2, 1.0 - (xx2 + zz2), yz2 + wx2);
    m[2] = float3(xz2 + wy2, yz2 - wx2, 1.0 - (xx2 + yy2));
    return m;
}

VSOut VS(VSIn vin, uint instanceId : SV_InstanceID)
{
    Voxel vox = gVoxels[instanceId];

    float4 worldCenter = float4(vox.center, 1.0);
    float4 viewPos = mul(worldCenter, gView);
    float4 clipCenter = mul(viewPos, gProj);

    float radius = length(vox.halfSize);
    float radiusView = radius / max(0.001, viewPos.z);

    // Approximate projected radius assuming perspective divide
    float2 projScale = float2(gProj[0][0], gProj[1][1]);
    float2 ndcExtent = radiusView * projScale;

    float2 offset = vin.pos * ndcExtent;
    float4 clipPos = clipCenter;
    clipPos.xy += offset * clipPos.w; // scale by w to stay in clip space

    VSOut vout;
    vout.pos = clipPos;
    vout.voxelIndex = instanceId;
    return vout;
}

struct PSOut
{
    float4 color : SV_Target0;
    float depth : SV_Depth;
};

float3 TransformDirection(float3 dir, float3x3 m)
{
    return mul(dir, m);
}

float3x3 Transpose3x3(float3x3 m)
{
    return float3x3(
        m[0].x, m[1].x, m[2].x,
        m[0].y, m[1].y, m[2].y,
        m[0].z, m[1].z, m[2].z);
}

PSOut PS(VSOut pin)
{
    PSOut pout;

    // Reconstruct view ray
    float2 ndc;
    ndc.x = (pin.pos.x / gViewportSize.x) * 2.0 - 1.0;
    ndc.y = -((pin.pos.y / gViewportSize.y) * 2.0 - 1.0); // DirectX NDC Y is inverted

    float4 clip = float4(ndc, 1.0, 1.0);
    float4 viewRay = mul(clip, gInvProj);
    viewRay /= viewRay.w;

    float3 rayDirView = normalize(viewRay.xyz);
    float3 rayDirWorld = mul(float4(rayDirView, 0.0), gInvView).xyz;
    float3 rayOriginWorld = gCameraPos;

    Voxel vox = gVoxels[pin.voxelIndex];

    float3x3 rot = QuaternionToMatrix(vox.rotation);
    float3x3 invRot = Transpose3x3(rot); // unit quaternion

    float3 localOrigin = mul(rayOriginWorld - vox.center, invRot);
    float3 localDir = mul(rayDirWorld, invRot);

    // Ray-box intersection (slab method, assumes ray starts outside)
    float3 invDir = 1.0 / max(abs(localDir), float3(1e-5, 1e-5, 1e-5)) * sign(localDir);
    float3 t1 = (-vox.halfSize - localOrigin) * invDir;
    float3 t2 = (vox.halfSize - localOrigin) * invDir;
    float3 tmin3 = min(t1, t2);
    float3 tmax3 = max(t1, t2);
    float tmin = max(tmin3.x, max(tmin3.y, tmin3.z));
    float tmax = min(tmax3.x, min(tmax3.y, tmax3.z));

    if (tmax < max(tmin, 0.0))
        discard;

    float tHit = tmin > 0.0 ? tmin : tmax;
    float3 localHit = localOrigin + tHit * localDir;

    // Compute normal by checking dominant axis
    float3 absHit = abs(localHit);
    float3 n = 0.0.xxx;
    if (absHit.x > absHit.y && absHit.x > absHit.z)
        n = float3(sign(localHit.x), 0, 0);
    else if (absHit.y > absHit.z)
        n = float3(0, sign(localHit.y), 0);
    else
        n = float3(0, 0, sign(localHit.z));

    float3 worldNormal = normalize(mul(n, rot));
    float3 worldPos = rayOriginWorld + tHit * rayDirWorld;

    float NdotL = max(dot(worldNormal, -normalize(gLightDir)), 0.0);
    float3 baseColor = vox.color.rgb;
    float3 color = baseColor * (0.2 + 0.8 * NdotL);

    float4 clipPos = mul(float4(worldPos, 1.0), gView);
    clipPos = mul(clipPos, gProj);
    float depth = clipPos.z / clipPos.w;

    pout.color = float4(color, 1.0);
    pout.depth = depth;
    return pout;
}
