
uniform vec2 resolution;
uniform vec3 origin;
uniform mat3 orientation;
uniform float plane_height;
uniform float iTime;

float sgn(float x) {
	return (x<0.)?-1.:1.;
}

float vmax(vec3 v) {
	return max(max(v.x, v.y), v.z);
}
float fSphere(vec3 p, float r)
{
	return length(p) - r;
}
float fBox(vec3 p, vec3 b) {
	vec3 d = abs(p) - b;
	return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
}
float fPlane(vec3 p, vec3 n) {
	return dot(p, -n);
}
// Cylinder standing upright on the xz plane
float fCylinder(vec3 p, float r, float height) {
	float d = length(p.xz) - r;
	d = max(d, abs(p.y) - height);
	return d;
}

// Repeat space along one axis. Use like this to repeat along the x axis:
// <float cell = pMod1(p.x,5);> - using the return value is optional.
float pMod1(inout float p, float size) {
	float halfsize = size*0.5;
	float c = floor((p + halfsize)/size);
	p = mod(p + halfsize, size) - halfsize;
	return c;
}
// Mirror at an axis-aligned plane which is at a specified distance <dist> from the origin.
float pMirror (inout float p, float dist) {
	float s = sgn(p);
	p = abs(p)-dist;
	return s;
}
// Repeat only a few times: from indices <start> to <stop> (similar to above, but more flexible)
float pModInterval1(inout float p, float size, float start, float stop) {
    float halfsize = size*0.5;
    float c = floor((p + halfsize)/size);
    p = mod(p+halfsize, size) - halfsize;
    if (c > stop) { //yes, this might not be the best thing numerically.
        p += size*(c - stop);
        c = stop;
    }
    if (c <start) {
        p += size*(c - start);
        c = start;
    }
    return c;
}

struct hitInfo
{
    float dist;
    int matId;
};
hitInfo min_h(hitInfo a, hitInfo b)
{
    if (a.dist < b.dist)
        return a;
    return b;
}

hitInfo map(vec3 p)
{
    hitInfo f = hitInfo( 1000., -1 );

    f = min_h(f, hitInfo(-fSphere(p, 50.), 0) );

    // the sign
    f = min_h(f, hitInfo(fBox(-p, vec3(5, 2, 0.2)), 1) );

    // the pole
    f = min_h(f, hitInfo(fCylinder(vec3(0, -10., -0.4) - p, 0.2, 10.), 1) );

    // the weird cage thing underneaths
    vec3 cage_p = p;
    pModInterval1(cage_p.x, 0.45, -10., 10.);
    f = min_h(f, hitInfo(fCylinder(vec3(0, -2, 0.4).xzy - cage_p.xzy, 0.01, 0.4), 1) );
    f = min_h(f, hitInfo(fCylinder(vec3(0, -2, 0.4).yxz - p.yxz, 0.02, 5.), 1) );
    f = min_h(f, hitInfo(fCylinder(vec3(0, -2, 0.55).yxz - p.yxz, 0.02, 5.), 1) );
    
    return f;
}
vec3 getAlbedo(vec3 p, int matId)
{
    if (matId == 0)
    {
        return vec3(0.1, 0.1, 0.1);
    }
    return vec3(1);
}

vec3 color(float t)
{
    vec3 a = vec3(0.660, 0.560, 0.680);
    vec3 b = vec3(0.718, 0.438, 0.720);
    vec3 c = vec3(0.520, 0.800, 0.520);
    vec3 d = vec3(-0.430, -0.397, -0.083);
    
    vec3 ret = a + b * cos(2.0 * 3.14159 * (c * t + d));
    return clamp(ret, 0.0, 1.0);
}

float castRay(vec3 o, vec3 ray, int iters, out hitInfo out_info)
{
    float t_max = 800.0;
    float t_min = 0.0;
    
    float omega = 1.2;
    float t = t_min;
    float candidate_error = 8000.0;
    float candidate_t = t_min;
    float previousRadius = 0.0;
    float stepLength = 0.0;
    float fsign = map(o).dist < 0.0 ? -1.0 : 1.0;
    //float fsign = 1.0;
    for (int i = 0; i < iters; ++i)
    {
        hitInfo info = map(o + ray * t);
        
        float signedRadius = fsign * info.dist;
        float radius = abs(signedRadius);
        
        bool sorFail = omega > 1.0 && (radius + previousRadius) < stepLength;
        if (sorFail)
        {
            stepLength -= omega * stepLength;
            omega = 1.0;
        }
        else
        {
            stepLength = signedRadius * omega;
        }
        
        previousRadius = radius;
        float error = radius / t;
        
        if (!sorFail && error < candidate_error)
        {
            candidate_t = t;
            candidate_error = error;
            out_info = info;
        }
        
        if (!sorFail && error < 0.25/resolution.y || t > t_max)
            break;
        
        t += stepLength;
    }
    
    /*if ((t > t_max || candidate_error > 0.25/resolution.y))
        return t_max;*/
    
    return candidate_t;
}

vec3 mapGradient(vec3 p)
{
    vec3 eps = vec3( 0.001, 0.0, 0.0 );
    vec3 ret;
    ret.x = map(p + eps.xyy).dist - map(p - eps.xyy).dist;
    ret.y = map(p + eps.yxy).dist - map(p - eps.yxy).dist;
    ret.z = map(p + eps.yyx).dist - map(p - eps.yyx).dist;
    return ret / (2. *eps.x);
}

vec3 mainImage(vec3 origin, vec3 ray)
{
	hitInfo castInfo;
    float t = castRay(origin, ray, 128, castInfo);

    {
        vec3 n = vec3(0,1,0);
        float d = dot(n, vec3(0,plane_height,0)-origin) / dot(n, ray);
        if (d < t && d > 0.)
        {
            vec3 p = origin + ray * d;
            float dist = map(p).dist;
            //vec3 final = mix(vec3(color(pow(sin(dist / 5.), 2.)) * mod(dist, 1.)), vec3(0.4), min(1., dist / 50.) );
            vec3 final = mix(vec3(0.8) * mod(dist, 1.), vec3(0.4), min(1., dist / 50.) );

            if (length(mapGradient(p)) > 1.01)
                final.yz = vec2(0);
            if (length(mapGradient(p)) < 0.99)
                final.xy = vec2(0);

            final = mix(vec3(0,0.8,0), final, smoothstep(0.0, 0.1, abs(p.z)) );
            return final;
        }
    }

    vec3 pt = origin + t * ray;
    vec3 normal = normalize(mapGradient(pt));

    if (castInfo.dist > 1.)
    	return vec3(0);

    vec3 albedo = getAlbedo(pt, castInfo.matId);

    vec3 final = albedo * (dot(normalize(vec3(1,2,3)), normal) * 0.5 + 0.5);

    return final;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.y;

    vec3 pxlLoc = 2.0 * vec3(uv, -2) - vec3(resolution.x / resolution.y, 1, 0);

    pxlLoc = orientation * pxlLoc;

    pxlLoc += origin;
    //vec3 origin = vec3(0, 0, 2);
    vec3 final = vec3(0);

    vec2 subpixel = vec2(0.5 / resolution.y, 0);
    final += mainImage(origin, normalize(pxlLoc + subpixel.yyy  - origin));
    /*final += mainImage(origin, normalize(pxlLoc + subpixel.xyy  - origin));
    final += mainImage(origin, normalize(pxlLoc + subpixel.yxy  - origin));
    final += mainImage(origin, normalize(pxlLoc - subpixel.xyy  - origin));
    final += mainImage(origin, normalize(pxlLoc - subpixel.yxy  - origin));
    final /= 5.;*/
    
    //final = vec3(gl_FragCoord.x / resolution.x);
    final = pow(final, vec3(1./2.2));

    //if (error > 1.)
    //	final = vec3(0);

    gl_FragColor = vec4(final, 1.);
    //gl_FragColor = vec4(gl_FragCoord.x / resolution.x);
}