#define V2 ImVec2
// void FX(ImDrawList* d, ImVec2 a, ImVec2 b, ImVec2 sz, ImVec4 mouse, float t)
// {
//   for (int n = 0; n < (1.0f + sinf(t * 5.7f)) * 40.0f; n++)
//     d->AddCircle(V2(a.x + sz.x * 0.5f, a.y + sz.y * 0.5f), sz.y * (0.01f + n * 0.03f), 
//         IM_COL32(255, 140 - n * 4, n * 3, 255));
// }
void FX(ImDrawList* d, V2 a, V2 b, V2 sz, ImVec4, float t0)
{
    for (float t = t0; t < t0 + 1.0f; t += 0.5f)
    {
        V2 cp0(a.x, b.y);
        V2 cp1(b);
        float ts = t - t0;
        cp0.x += (0.4f + sin(t) * 0.3f) * sz.x;
        cp0.y -= (0.5f + cos(ts * ts) * 0.4f) * sz.y;
        cp1.x -= (0.4f + cos(t) * 0.4f) * sz.x;
        cp1.y -= (0.5f + sin(ts * t) * 0.3f) * sz.y;
        d->AddBezierCubic(V2(a.x + 10.0f, b.y - 10.0f), cp0, cp1, V2(b.x - 10.0f, b.y - 10.0f), IM_COL32(100 + ts*150, 255 - ts*150, 60, ts * 200), 20.0f);
    }
}
