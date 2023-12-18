// Max constant buffer size is 4096 float4's
// Our current InstanceData is basically 5 float4's
// 4096 / 5 = 819.2
#define MAX_INSTANCES 819

struct InstanceData
{
    float4x4 World;
    uint MaterialIndex;
    uint Pad0;
    uint Pad1;
    uint Pad2;
};