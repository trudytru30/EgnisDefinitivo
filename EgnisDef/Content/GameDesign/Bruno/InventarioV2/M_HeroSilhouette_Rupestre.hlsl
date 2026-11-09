// =====================================================================
//  M_HeroSilhouette - Estilo rupestre (código del nodo Custom)
//  Output Type: CMOT Float 4
//
//  Entradas del nodo Custom (las 9 primeras ya existen):
//    Tex, UV, Width, FillColor, LineColor, GlowColor, Locked, LockedColor, LockedRim
//  Entradas NUEVAS (crear parámetro y conectarlo con este mismo nombre):
//    ShadowPigment  VectorParameter  (0.08, 0.05, 0.04, A=0.6)  segundo pigmento (manchas)
//    EdgeNoise      ScalarParameter  0.35   irregularidad del borde (0 = limpio)
//    Wear           ScalarParameter  0.30   desgaste del pigmento (0 = fresco, 1 = casi borrado)
//    Seed           ScalarParameter  0.0    variación del ruido (distinto por héroe si quieres)
//
//  Colores recomendados (paleta paleolítica):
//    FillColor  ocre rojo      (0.55, 0.17, 0.07, 1.0)
//    LineColor  carbón         (0.05, 0.04, 0.035, 0.9)
//    GlowColor  ocre soplado   (0.60, 0.25, 0.10, 0.35)
// =====================================================================

struct RupFn
{
    float hash(float2 p)
    {
        p = frac(p * float2(123.34, 456.21));
        p += dot(p, p + 45.32);
        return frac(p.x * p.y);
    }
    float noise(float2 p)
    {
        float2 i = floor(p);
        float2 f = frac(p);
        float2 u = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(hash(i), hash(i + float2(1, 0)), u.x),
                    lerp(hash(i + float2(0, 1)), hash(i + float2(1, 1)), u.x), u.y);
    }
    float fbm(float2 p)
    {
        float v = 0.0;
        float a = 0.5;
        for (int k = 0; k < 4; k++) { v += a * noise(p); p *= 2.03; a *= 0.5; }
        return v;
    }
};
RupFn R;

// --- 1. Borde hecho a mano: deformar las UV con ruido ---
float2 sp  = UV * 8.0 + Seed * 17.0;
float2 wob = float2(R.fbm(sp), R.fbm(sp + 31.7)) - 0.5;
float2 uvw = UV + wob * EdgeNoise * Width * 6.0;

float c = 1 - Texture2DSample(Tex, TexSampler, uvw).a;

// --- 2. Contorno y halo con grosor irregular (trazo de dedo / pincel) ---
float dil  = c;
float glow = 0;
for (int i = 0; i < 16; i++)
{
    float  an  = i * 0.39269908;
    float2 d   = float2(cos(an), sin(an));
    float  jit = 0.6 + 0.8 * R.hash(uvw * 400.0 + i);
    dil   = max(dil, 1 - Texture2DSample(Tex, TexSampler, uvw + d * Width * jit).a);
    dil   = max(dil, 1 - Texture2DSample(Tex, TexSampler, uvw + d * Width * 0.5 * jit).a);
    glow += 1 - Texture2DSample(Tex, TexSampler, uvw + d * Width * 2.5 * jit).a;
    glow += 1 - Texture2DSample(Tex, TexSampler, uvw + d * Width * 5.0 * jit).a;
}
glow = saturate(glow / 32.0) * (1 - c);

// Halo "soplado" (como las manos en negativo): moteado de pigmento
float speck = R.hash(floor(UV * 350.0) + Seed);
glow *= smoothstep(0.25, 0.9, speck + glow * 0.6);

// Fundido del halo hacia los bordes del retrato: se apaga antes de tocar el
// rectángulo de la imagen, con un borde irregular (pigmento soplado sobre piedra)
float edgeD = min(min(UV.x, 1.0 - UV.x), min(UV.y, 1.0 - UV.y));
float edgeN = (R.fbm(UV * 6.0 + Seed * 2.3) - 0.5) * 0.12;
glow *= smoothstep(0.0, 0.2, edgeD + edgeN);

float edge = saturate(dil - c);

// --- 3. Grano de roca, manchas y pincel seco ---
float grain  = R.fbm(UV * 40.0 + Seed * 3.1);                    // grano fino de la piedra
float big    = R.fbm(UV * 3.0  + Seed * 5.7);                    // manchas grandes
float streak = R.fbm(float2(UV.x * 60.0, UV.y * 6.0) + Seed);    // vetas de pincel seco

float wearMask = saturate((grain * 0.6 + streak * 0.4) - Wear * 0.8 + 0.4);
wearMask = smoothstep(0.2, 0.6, wearMask);

float  shadeMix = smoothstep(0.35, 0.7, big);
float3 pigment  = lerp(FillColor.rgb, ShadowPigment.rgb, shadeMix * ShadowPigment.a);
pigment *= 0.85 + 0.3 * grain;

float cov     = c * wearMask;
float edgeCov = edge * smoothstep(0.15, 0.55, R.fbm(UV * 25.0 + Seed * 9.3) + 0.25); // línea de carbón rota

// --- 4. Composición (misma lógica que el original) ---
float4 p = 0;
if (Locked > 0.5)
{
    float3 lc = LockedColor.rgb * (0.85 + 0.3 * grain);
    float a1 = cov * LockedColor.a;    p.rgb = lc * a1            + p.rgb * (1 - a1); p.a = a1 + p.a * (1 - a1);
    float a2 = edgeCov * LockedRim.a;  p.rgb = LockedRim.rgb * a2 + p.rgb * (1 - a2); p.a = a2 + p.a * (1 - a2);
}
else
{
    float a1 = glow * GlowColor.a;     p.rgb = GlowColor.rgb * a1 + p.rgb * (1 - a1); p.a = a1 + p.a * (1 - a1);
    float a2 = cov * FillColor.a;      p.rgb = pigment * a2       + p.rgb * (1 - a2); p.a = a2 + p.a * (1 - a2);
    float a3 = edgeCov * LineColor.a;  p.rgb = LineColor.rgb * a3 + p.rgb * (1 - a3); p.a = a3 + p.a * (1 - a3);
}
return float4(p.rgb / max(p.a, 0.0001), p.a);
