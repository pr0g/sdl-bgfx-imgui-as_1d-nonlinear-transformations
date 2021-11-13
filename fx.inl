#define V2 ImVec2

void FX(ImDrawList* d, ImVec2 a, ImVec2 b, ImVec2 sz, ImVec4 mouse, float t)
{
  for (int n = 0; n < (1.0f + sinf(t * 5.7f)) * 40.0f; n++)
    d->AddCircle(V2(a.x + sz.x * 0.5f, a.y + sz.y * 0.5f), sz.y * (0.01f + n * 0.03f),
        IM_COL32(255, 140 - n * 4, n * 3, 255));
}
