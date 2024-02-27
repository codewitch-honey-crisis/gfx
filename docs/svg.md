#### [← Back to index](index.md)

<a name="5"></a>

# 5. Scalable Vector Graphics (SVG)

Scalable Vector Graphics is a web standard for rendering vector based graphics elements. It's defined in XML, but GFX also allows you to build SVG elements directly without using an XML intermediary.

SVG rasterization supports alpha blending with anti-aliasing, so the generated graphics are not only scalable, but the resulting images are smooth and blended appropriately.

<a name="5.1"></a>

## 5.1 SVG Document

The `svg_doc` class holds a parsed or built out SVG graphic which can be rendered and scaled.

To parse an SVG document you would use it like

```cpp
file_stream fs("/spiffs/bee.svg");
svg_doc bee;
// should check the error result here:
svg_doc::read(&fs,&bee);
```

<a name="5.2"></a>

## 5.2 Builder Classes

The builder classes allow you to create SVG documents dynamically. Using them is a bit involved, as SVG elements have a lot of configurable parameters.

```cpp
using svg_color = color<rgba_pixel<32>>;
svg_doc doc;
svg_doc_builder b(sizef(100, 50));
svg_shape_info si;
si.stroke_width = 1;
si.stroke.type = svg_paint_type::color;
si.fill.type = svg_paint_type::color;

si.stroke.color = svg_color::black;
si.fill.color = svg_color::blue;

rectf bb(bounds.x1, bounds.y1, bounds.x2, bounds.y2);
b.add_rectangle(bb, si);
b.to_doc(&doc);
```
[→ Drawing](drawing.md)

[← Fonts](fonts.md)

