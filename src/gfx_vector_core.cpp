#include <gfx_vector_core.hpp>
namespace gfx {
matrix::matrix(float a, float b, float c, float d, float e, float f) :
        a(a),b(b),c(c),d(d),e(e),f(f) {

}
matrix::matrix() {

}
matrix::matrix(const matrix& rhs) {
    copy_from(rhs);
}
matrix& matrix::operator=(const matrix& rhs) {
    copy_from(rhs);
    return *this;
}
matrix::matrix(matrix&& rhs) {
    copy_from(rhs);
}
matrix& matrix::operator=(matrix&& rhs) {
    copy_from(rhs);
    return *this;
}

matrix matrix::create_identity() {
    return matrix(1,0,0,1,0,0);
}
void matrix::copy_from(const matrix& rhs) {
    a=rhs.a;
    b=rhs.b;
    c=rhs.c;
    d=rhs.d;
    e=rhs.e;
    f=rhs.f;
}
matrix matrix::operator*(const matrix& rhs) {
    float a = this->a * rhs.a + this->b * rhs.c;
    float b = this->a * rhs.b + this->b * rhs.d;
    float c = this->c * rhs.a + this->d * rhs.c;
    float d = this->c * rhs.b + this->d * rhs.d;
    float e = this->e * rhs.a + this->f * rhs.c + rhs.e;
    float f = this->e * rhs.b + this->f * rhs.d + rhs.f;
    return matrix(a,b,c,d,e,f);
}

matrix& matrix::operator*=(const matrix& rhs) {
    float a = this->a * rhs.a + this->b * rhs.c;
    float b = this->a * rhs.b + this->b * rhs.d;
    float c = this->c * rhs.a + this->d * rhs.c;
    float d = this->c * rhs.b + this->d * rhs.d;
    float e = this->e * rhs.a + this->f * rhs.c + rhs.e;
    float f = this->e * rhs.b + this->f * rhs.d + rhs.f;
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
    this->e = e;
    this->f = f;
    return *this;
}
matrix matrix::translate(float x, float y) {
    matrix result = *this;
    result=create_translate(x,y) * result;
    return result;
}
matrix matrix::scale(float xs, float ys) {
    matrix result = *this;
    result=create_scale(xs,ys) * result;
    return result;
}
matrix matrix::skew(float xa, float ya) {
    matrix result = *this;
    result=create_skew(xa,ya) * result;
    return result;
}
matrix matrix::skew_x(float angle) {
    matrix result = *this;
    result=create_skew_x(angle) * result;
    return result;
}
matrix matrix::skew_y(float angle) {
    matrix result = *this;
    result=create_skew_y(angle) * result;
    return result;
}
matrix matrix::rotate(float angle) {
    matrix result = *this;
    result=create_rotate(angle) * result;
    return result;
}


matrix& matrix::translate_inplace(float x, float y) {
    *this=create_translate(x,y) * *this;
    return *this;
}
matrix& matrix::scale_inplace(float xs, float ys) {
    *this=create_scale(xs,ys) * *this;
    return *this;
}
matrix& matrix::skew_inplace(float xa, float ya) {
    *this=create_skew(xa,ya) * *this;
    return *this;
}
matrix& matrix::skew_x_inplace(float angle) {
    *this=create_skew_x(angle) * *this;
    return *this;
}
matrix& matrix::skew_y_inplace(float angle) {
    *this=create_skew_y(angle) * *this;
    return *this;
}
matrix& matrix::rotate_inplace(float angle) {
    *this=create_rotate(angle) * *this;
    return *this;
}

matrix matrix::create_scale(float x, float y) {
    return matrix(x,0.0f,0.0f,y,0.0f,0.0f);
}
matrix matrix::create_skew(float xa, float ya) {
     return matrix(1, tanf(ya), tanf(xa), 1, 0, 0);
}
matrix matrix::create_skew_x(float a) {
     return matrix(1, 0, tanf(a), 1, 0, 0);
}
matrix matrix::create_skew_y(float a) {
     return matrix(1, tanf(a),0, 1, 0, 0);
}
matrix matrix::create_translate(float x, float y) {
    return matrix(1.0f,0.0f,0.0f,1.0f,x,y);
}
matrix matrix::create_rotate(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return matrix( c, s, -s, c, 0, 0);
}
matrix matrix::create_fit_to(sizef dimensions,const rectf& bounds) {
    float dst_width = bounds.width();
    float dst_height = bounds.height();
    float scale_x = dst_width / dimensions.width;
    float scale_y = dst_height / dimensions.height;
    matrix result = create_translate(bounds.x1, bounds.y1);
    return create_scale(scale_x, scale_y) * result;
}
bool matrix::invert(const matrix& rhs, matrix* out_inverted) {
    float det = (rhs.a * rhs.d - rhs.b * rhs.c);
    if(det == 0.f)
        return false;
    if(out_inverted!=nullptr) {
        float inv_det = 1.f / det;
        float a = rhs.a * inv_det;
        float b = rhs.b * inv_det;
        float c = rhs.c * inv_det;
        float d = rhs.d * inv_det;
        float e = (rhs.c * rhs.f - rhs.d * rhs.e) * inv_det;
        float f = (rhs.b * rhs.e - rhs.a * rhs.f) * inv_det;
        *out_inverted = matrix(d,-b,-c,a,e,f);
    }
    return true;
}
void matrix::map(float x,float y,float* out_xx, float* out_yy) const {
    *out_xx = x * a + y * c + e;
    *out_yy = x * b + y * d + f;
}
void matrix::map_point(const pointf& src,pointf* out_dst) const {
    map(src.x,src.y,&out_dst->x,&out_dst->y);
}
void matrix::map_points(const pointf* src,pointf* out_dst, size_t size) const {
    for(size_t i = 0; i<size;++i) {
        map_point(src[i],&out_dst[i]);
    }
}
void matrix::map_rect(const rectf& src, rectf* out_dst) const {
    map(src.x1,src.y1,&out_dst->x1,&out_dst->y1);
    map(src.x2,src.y2,&out_dst->x2,&out_dst->y2);
}
}