#ifndef XE_GTAO_CALLBACKS_HLSL
#define XE_GTAO_CALLBACKS_HLSL

#include "../Common.hlsli"
#include "../ShaderTypes.h"

// The vendored algorithm refers to the constants block unqualified at global scope.
typedef Gleam::GTAOConstants GTAOConstants;

#define XE_GTAO_DEPTH_MIP_LEVELS 4
#define XE_GTAO_NUMTHREADS_X 8
#define XE_GTAO_NUMTHREADS_Y 8

#define XE_GTAO_FP32_DEPTHS 1
#define XE_GTAO_USE_HALF_FLOAT_PRECISION 0

#ifndef XE_GTAO_USE_DEFAULT_CONSTANTS
#define XE_GTAO_USE_DEFAULT_CONSTANTS 1
#endif

// some constants reduce performance if provided as dynamic values; if these constants are not required to be dynamic and they match default values, 
// set XE_GTAO_USE_DEFAULT_CONSTANTS and the code will compile into a more efficient shader
#define XE_GTAO_DEFAULT_RADIUS_MULTIPLIER               (1.457f  )  // allows us to use different value as compared to ground truth radius to counter inherent screen space biases
#define XE_GTAO_DEFAULT_FALLOFF_RANGE                   (0.615f  )  // distant samples contribute less
#define XE_GTAO_DEFAULT_SAMPLE_DISTRIBUTION_POWER       (2.0f    )  // small crevices more important than big surfaces
#define XE_GTAO_DEFAULT_THIN_OCCLUDER_COMPENSATION      (0.0f    )  // the new 'thickness heuristic' approach
#define XE_GTAO_DEFAULT_FINAL_VALUE_POWER               (2.2f    )  // modifies the final ambient occlusion value using power function - this allows some of the above heuristics to do different things
#define XE_GTAO_DEFAULT_DEPTH_MIP_SAMPLING_OFFSET       (3.30f   )  // main trade-off between performance (memory bandwidth) and quality (temporal stability is the first affected, thin objects next)

#define XE_GTAO_OCCLUSION_TERM_SCALE                    (1.5f)      // for packing in UNORM (because raw, pre-denoised occlusion term can overshoot 1 but will later average out to 1)

#endif // XE_GTAO_CALLBACKS_HLSL
