// =====================================================================
// M_UI_Frost - Custom node source (reference copy of the code inside
// the "FrostCore" Custom expression of M_UI_Frost).
// Output: float4 (RGB = final color, A = opacity). Domain: User Interface.
//
// Inputs:
//   UV, TimeSec, Amount (FrostAmount 0..1), PatternScale, Opacity,
//   EdgeThickness, InnerFade, AnimSpeed, IdleStrength, Seed, WidgetAspect, SparkleStrength (FrostSparkleStrength 0..1), FrostTint (rgb),
//   BaseRGB, BaseA (BaseTexture sample), BaseTint (rgb), BaseTintA
//
// Two ways to use it:
//   Overlay : BaseTint.a = 0  -> only frost is drawn (put an Image on top
//             of any widget). BaseTexture alpha still clips the frost, so
//             assigning the element's texture keeps its shape.
//   Brush   : BaseTint.a = 1 + BaseTexture = element texture -> the element
//             itself is drawn and frozen ("frozen glass" tint underneath).
// =====================================================================

struct FrostLib
{
    // "Hash without sine" (small multipliers): stays precise for large coordinates,
    // so the noise never degenerates into straight stripes / vertical cuts.
    float Hash21(float2 p)
    {
        float3 p3 = frac(float3(p.xyx) * 0.1031);
        p3 += dot(p3, p3.yzx + 33.33);
        return frac((p3.x + p3.y) * p3.z);
    }
    float2 Hash22(float2 p)
    {
        float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
        p3 += dot(p3, p3.yzx + 33.33);
        return frac((p3.xx + p3.yz) * p3.zy);
    }
    float ValueNoise(float2 p)
    {
        float2 i = floor(p);
        float2 f = frac(p);
        float2 u = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(Hash21(i), Hash21(i + float2(1, 0)), u.x),
                    lerp(Hash21(i + float2(0, 1)), Hash21(i + float2(1, 1)), u.x), u.y);
    }
    // Smooth fractal noise: drives the irregular growth front
    float Fbm(float2 p)
    {
        float v = 0.0, a = 0.5;
        for (int k = 0; k < 5; k++)
        {
            v += a * ValueNoise(p);
            p = mul(float2x2(1.6, 1.2, -1.2, 1.6), p);
            a *= 0.5;
        }
        return v;
    }
    // Ridged noise: thin feathery ice veins
    float Ridged(float2 p)
    {
        float v = 0.0, a = 0.5;
        for (int k = 0; k < 4; k++)
        {
            float n = 1.0 - abs(ValueNoise(p) * 2.0 - 1.0);
            v += a * n * n;
            p = mul(float2x2(1.6, 1.2, -1.2, 1.6), p) * 1.1;
            a *= 0.5;
        }
        return v;
    }
    // Voronoi cell borders: crystalline facets
    float VoronoiEdge(float2 p)
    {
        float2 i = floor(p);
        float2 f = frac(p);
        float d1 = 8.0, d2 = 8.0;
        for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++)
        {
            float2 g = float2(x, y);
            float2 r = g + Hash22(i + g) - f;
            float d = dot(r, r);
            if (d < d1) { d2 = d1; d1 = d; }
            else if (d < d2) { d2 = d; }
        }
        return sqrt(d2) - sqrt(d1);
    }
};
FrostLib F;

float amount = saturate(Amount);

// --- Aspect ratio (short side = 1) ---
// WidgetAspect (width/height) is sent by WBP_FrostElement and is exact and constant across the widget.
// Fallback when it is 0 (material used directly on an Image): screen derivatives, quantized so tiny
// per-pixel-block differences cannot shift the pattern (they caused vertical seams).
float pxU = 1.0 / max(length(float2(ddx(UV.x), ddy(UV.x))), 1e-5);
float pxV = 1.0 / max(length(float2(ddx(UV.y), ddy(UV.y))), 1e-5);
float aspect = WidgetAspect > 0.0 ? WidgetAspect : round(pxU / pxV * 16.0) / 16.0;
aspect = max(aspect, 0.01);
float2 dims = aspect >= 1.0 ? float2(aspect, 1.0) : float2(1.0, 1.0 / aspect);
float2 p = UV * dims;                              // aspect-correct space

// --- Distance to the nearest edge; soft-min makes corners freeze first ---
float2 e = min(UV, 1.0 - UV) * dims;
float k = 0.12;
float hk = saturate(0.5 + 0.5 * (e.y - e.x) / k);
float edgeDist = max(lerp(e.y, e.x, hk) - k * hk * (1.0 - hk), 0.0);

// --- Organic growth front (domain-warped noise) ---
float t = TimeSec * AnimSpeed;                            // TimeSec ignores game pause
// Seed only picks a small offset (bounded) so coordinates stay small and precise
float2 sp = p * 3.0 * PatternScale + float2(frac(Seed * 0.6180339), frac(Seed * 0.4142136)) * 19.0;
// Idle drift of the warp field: crystals and the front slowly crawl (cyclic, so it never grows over time)
float2 dr = float2(sin(t * 0.05), cos(t * 0.037)) * 3.0 * IdleStrength;
float2 warp = float2(F.Fbm(sp * 0.6 + float2(dr.x, -dr.y * 0.7)), F.Fbm(sp * 0.6 + 5.2 + float2(-dr.y * 0.8, dr.x * 0.5)));
float n = F.Fbm(sp + warp * 1.5);

float veins = F.Ridged(sp * 2.2 + warp * 2.0);           // feathery ice veins

// Growth curve: 0.25 thin edges, 0.5 moderate, 0.75 heavy, 1.0 almost full
float reach = pow(amount, 1.6) * 0.55 * EdgeThickness;
// Jagged border; the front also creeps forward along the veins (dendritic look)
float d = edgeDist + (n - 0.5) * 0.24 * (0.35 + reach) - (veins - 0.35) * 0.07 * (0.3 + amount);
// Idle "breathing": the frost front creeps in and out slowly, out of phase across the element
float breath = 0.5 * sin(t * 1.3 + n * 6.2831) + 0.5 * sin(t * 0.55 + veins * 9.0);
d += breath * 0.035 * IdleStrength * saturate(amount * 4.0);
float soft = 0.02 + 0.04 * amount;
float mask = (1.0 - smoothstep(reach - soft, reach + soft * 0.25, d)) * saturate(amount * 12.0);
float depth = saturate((reach - d) / max(reach, 1e-3)); // 1 at the edge, 0 at the front

// --- Crystalline detail ---
float facets = 1.0 - smoothstep(0.0, 0.05, F.VoronoiEdge(sp * 4.0 + warp));
float crystal = saturate(veins * veins * 1.4 + facets * 0.45);
// Fully solid at the border (100%), crystalline further in
float core = lerp(0.55 + 0.45 * crystal, 1.0, smoothstep(0.55, 1.0, depth));
// InnerFade: 0 = same opacity everywhere; 1 = fades progressively to fully transparent at the growth front
float innerFade = lerp(1.0, smoothstep(0.0, 1.0, depth), saturate(InnerFade));
// Opacity is the global multiplier (edges stay at 100% unless it is lowered)
float frostA = saturate(mask * core * innerFade * Opacity);

// --- Idle animation: light sheen sweeping across the ice (~every 4.5 s) ---
float sweepPos = dot(UV * dims, float2(1.0, 0.6)) / (dims.x + dims.y * 0.6);
float sweep = frac(t * 0.22) * 1.8 - 0.4;
float sheen = exp(-pow((sweepPos - sweep) / 0.07, 2.0)) * IdleStrength * (0.4 + 0.6 * crystal);

// --- Twinkling sparkles + shimmer ---
float2 gp = sp * 10.0;
float g = F.Hash21(floor(gp));
float twinkle = pow(0.5 + 0.5 * sin(t * 3.0 + g * 60.0), 4.0);
float glint = pow(g, 14.0) * twinkle
            * (1.0 - smoothstep(0.0, 0.3, length(frac(gp) - 0.5)))
            * mask * crystal * saturate(SparkleStrength);  // SparkleStrength: 0 = off, 1 = full
float shimmer = 0.9 + 0.1 * sin(t * 1.5 + n * 12.0);

float3 tint = FrostTint.rgb;
float3 frostRGB = lerp(tint * 0.75, float3(1, 1, 1), saturate(crystal * 0.7 + depth * 0.35)) * shimmer
                + glint * 1.8 + sheen * 0.5;
frostA = saturate(frostA * (1.0 + sheen * 0.6));

// --- Base element -> frozen glass (desaturated, cold tint under the frost) ---
float3 baseRGB = BaseRGB.rgb * BaseTint.rgb;
float baseA = BaseA * BaseTintA;
float lum = dot(baseRGB, float3(0.299, 0.587, 0.114));
float3 frozenBase = lerp(baseRGB, lerp(lum.xxx, baseRGB, 0.35) * tint * 1.15, mask * 0.65);

// Frost is clipped to the base texture's shape (alpha)
frostA *= BaseA;
float outA = frostA + baseA * (1.0 - frostA);
float3 outRGB = (frostRGB * frostA + frozenBase * baseA * (1.0 - frostA)) / max(outA, 1e-4);
outA = saturate(outA + glint * 0.6 * BaseA);

return float4(outRGB, outA);
