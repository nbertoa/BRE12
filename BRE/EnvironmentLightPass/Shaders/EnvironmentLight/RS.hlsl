#define RS \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
"DENY_GEOMETRY_SHADER_ROOT_ACCESS | " \
"DENY_HULL_SHADER_ROOT_ACCESS | " \
"DENY_DOMAIN_SHADER_ROOT_ACCESS), " \
"CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \
"CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \
"DescriptorTable(SRV(t0), SRV(t1), visibility = SHADER_VISIBILITY_PIXEL), " \
"DescriptorTable(SRV(t2), SRV(t3), visibility = SHADER_VISIBILITY_PIXEL), " \
"DescriptorTable(SRV(t4), visibility = SHADER_VISIBILITY_PIXEL), " \
"DescriptorTable(SRV(t5), visibility = SHADER_VISIBILITY_PIXEL), " \
"StaticSampler(s0, filter=FILTER_ANISOTROPIC)"