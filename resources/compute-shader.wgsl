struct Uniforms {
    kernel: mat3x3<f32>,
    filter_type: u32,
    frame: u32,
}

struct Storages {
    states: array<array<f32, 12>>,
}

const SCREEN_WIDTH: i32 = 1920;
const SCREEN_HEIGHT: i32 = 1080;

// @group(0) @binding(0) var inputTexture: texture_2d<f32>;
@group(0) @binding(0) var outputTexture: texture_storage_2d<rgba8unorm, write>;
@group(0) @binding(1) var<uniform> uniforms: Uniforms;
@group(0) @binding(2) var<storage, read_write> storages: Storages;


const N = 12u;
const S = 5000.;
const B = array<i32, 12> (-241,-221,-199,-91,-104,86,157,-79,-60,-82,-172,202);
const W = array<array<i32, 12>, 48>(
        array<i32, 12>(-684,-42,33,-11,52,375,159,45,92,-38,-105,330),         array<i32, 12>(-102,-412,12,30,143,249,138,132,74,-69,-2,203),         array<i32, 12>(87,38,-293,-173,415,-26,-130,141,132,81,80,-100),         array<i32, 12>(105,66,9,-547,-92,-163,-221,173,-42,-211,122,-122),         array<i32, 12>(-9,-97,-82,76,-1052,4,-276,34,-109,42,-96,22),         array<i32, 12>(-11,-195,-20,283,87,-1215,-389,368,-177,-143,-67,-92),         array<i32, 12>(7,-47,44,-76,221,279,-1182,-216,-226,152,-57,-268),         array<i32, 12>(-156,-175,51,104,203,16,108,-1341,94,91,-203,28),         array<i32, 12>(101,82,31,-56,-6,69,49,182,-1085,138,-10,-78),         array<i32, 12>(-294,-227,-53,-151,21,-39,72,-66,111,-880,-70,-108),         array<i32, 12>(-243,-243,-4,-19,41,-78,-79,210,12,-22,-1107,245),         array<i32, 12>(-245,-224,-40,95,108,-91,423,-174,60,-172,-51,-812),         array<i32, 12>(-4,-81,21,34,41,49,27,2,-28,-86,-67,58),         array<i32, 12>(80,156,-74,99,-16,27,25,-54,-22,-38,-40,6),         array<i32, 12>(18,74,474,215,-124,10,-56,88,52,-22,34,-115),         array<i32, 12>(-286,-220,-39,535,-139,-50,13,169,-209,-61,314,-351),         array<i32, 12>(-13,2,3,-18,-239,39,-74,-20,72,-101,33,49),         array<i32, 12>(93,75,11,9,70,119,-140,33,-7,140,5,58),         array<i32, 12>(114,74,33,-5,-142,2,-253,-129,-6,-104,-119,164),         array<i32, 12>(193,97,-9,45,24,193,45,200,-208,-53,-215,81),         array<i32, 12>(188,127,57,16,24,-24,-96,-46,-33,-90,-241,124),         array<i32, 12>(-196,-161,-23,192,-97,-74,111,-27,32,-143,27,-158),         array<i32, 12>(82,83,11,-5,76,46,77,-100,21,-89,-174,153),         array<i32, 12>(22,33,17,-55,41,-106,-41,-50,65,-88,158,-1),         array<i32, 12>(-18,-129,-60,-94,91,200,-297,369,402,-363,345,-124),         array<i32, 12>(-158,53,11,-369,-44,-63,-222,257,251,-144,304,-139),         array<i32, 12>(-131,-54,220,72,-314,92,126,-134,-124,-42,-46,98),         array<i32, 12>(-53,-18,-6,-70,250,143,25,-51,210,-7,111,-187),         array<i32, 12>(-100,-82,-3,-327,320,-315,-51,71,85,242,131,-24),         array<i32, 12>(-227,-188,-20,187,-121,-131,24,39,-219,144,79,81),         array<i32, 12>(-122,-93,-44,241,203,175,154,-55,-234,94,96,-20),         array<i32, 12>(-190,26,8,-196,26,256,-35,-505,-257,-155,19,-222),         array<i32, 12>(39,-8,-15,102,-79,-280,-63,203,-145,-466,-104,-17),         array<i32, 12>(9,-14,-77,-103,40,206,-171,299,-51,601,200,3),         array<i32, 12>(663,407,-22,673,232,396,212,-262,256,-30,-167,-118),         array<i32, 12>(225,247,88,387,315,71,110,-171,-521,-78,-309,-6),         array<i32, 12>(-7,24,0,-58,18,-6,-11,-35,18,-2,-44,8),         array<i32, 12>(12,-3,7,-8,12,0,41,18,21,-18,-30,51),         array<i32, 12>(168,28,26,231,8,4,81,13,-246,4,-187,-37),         array<i32, 12>(-61,-70,-9,86,131,-113,-98,195,199,-154,184,-44),         array<i32, 12>(4,-25,-19,-76,0,-27,86,201,17,-46,-43,-52),         array<i32, 12>(-65,-50,-26,-35,-55,-21,-7,96,17,69,174,-79),         array<i32, 12>(92,121,53,35,35,167,-145,-169,98,67,-56,169),         array<i32, 12>(24,-13,-10,-82,48,147,75,66,36,84,-19,34),         array<i32, 12>(146,105,49,-20,19,150,-53,-88,-52,-17,-166,91),         array<i32, 12>(150,113,20,-24,-32,-16,-78,-50,-170,35,6,27),         array<i32, 12>(-113,-88,2,95,7,-107,-12,76,8,-48,20,-8),         array<i32, 12>(-54,-52,-46,29,-67,-39,86,43,-35,213,22,-82), );


var<private> current_index: vec2<i32>;


fn R(dx: i32, dy: i32, c: u32) -> f32 {
    let x = (current_index.x + dx + SCREEN_WIDTH) % SCREEN_WIDTH;
    let y = (current_index.y + dy + SCREEN_HEIGHT) % SCREEN_HEIGHT;
    let i = x + y*SCREEN_WIDTH;
    return storages.states[i][c];
}

fn sobx(c: u32) -> f32 {
    return R(-1, 1, c) + R(-1, 0, c)*2.0 + R(-1,-1, c)
          -R( 1, 1, c) - R( 1, 0, c)*2.0 - R( 1,-1, c);
}

fn soby(c: u32) -> f32 {
    return R( 1, 1, c)+R( 0, 1, c)*2.0+R(-1, 1, c)
          -R( 1,-1, c)-R( 0,-1, c)*2.0-R(-1,-1, c);
}


fn lap(c: u32) -> f32 {
    return R(1,1,c)+R(1,-1,c)+R(-1,1,c)+R(-1,-1,c)
        +2.0* ( R(0,1,c)+R(0,-1,c)+R(1,0,c)+R(-1,0,c) ) - 12.0*R(0, 0,c);
}

fn update(ys: array<f32, N>, ps: array<f32, N>) -> array<f32, N> {
  // for some reason, accessing consts is very expensive, hence local vars
  // see https://bugs.chromium.org/p/ti32/issues/detail?id=2032
  var ws = W;
  var bs = B;

  var ys_v = ys;
  var ps_v = ps; // vulkan target in naga does not allow indexing an argument array

  // construct hidden state
  var hs = array<f32, 48>();
  for (var i = 0u; i<N; i++) {
    hs[i] = ys_v[i];
    hs[i+N] = ps_v[i];
    hs[i+N*2u] = abs(ys_v[i]);
    hs[i+N*3u] = abs(ps_v[i]);
  }

  // do 1x1 conv
  var dy = array<f32, N>();
  for (var c = 0u; c < N; c++) {
      var us = f32(bs[c]);

      for (var i = 0u; i < 48u; i++) {
          us += hs[i] * f32(ws[i][c]);
      }
      dy[c] = us / S;
  }

  return dy;
}

fn get_index(idx : u32, idy: u32, screen_size: vec2u) -> i32 {
    return  i32(idx) + i32(idy) * SCREEN_WIDTH;
}


@compute @workgroup_size(16, 16)
fn main_image(@builtin(global_invocation_id) id: vec3u) {
    // setup
    let screen_size = vec2u(textureDimensions(outputTexture));
    if (id.x >= screen_size.x || id.y >= screen_size.y) { return; }
    current_index = vec2i(i32(id.x), i32(id.y));

    // initial state
    if (uniforms.frame == 1) {
        for (var s=0u; s<N; s++) {
            // let i = id.x + id.y * SCREEN_WIDTH;
            let i = get_index(id.x, id.y, screen_size);
            let rand = fract(sin(f32(f32(i) + f32(N) * f32(s)) / f32(SCREEN_WIDTH)) * 43758.5453123) + .5;
            storages.states[i][s] = floor(rand);
        }

    return;
    }


    // construct state + perception vectors
    var xs = array<f32, N>();
    for (var c=0u; c<N; c++) {
        // let i = id.x + id.y * SCREEN_WIDTH;
        let i = get_index(id.x, id.y, screen_size);
        xs[c] = storages.states[i][c];
    }

    var ps = array<f32, 12>(
        lap(0u),
        lap(1u),
        lap(2u),
        lap(3u),

        sobx(4u),
        sobx(5u),
        sobx(6u),
        sobx(7u),

        soby(8u),
        soby(9u),
        soby(10u),
        soby(11u)
    );

    // update state
    var dx = update(xs, ps);

    // save state
    for (var s=0u; s<N; s++) {
        // let i = id.x + id.y * SCREEN_WIDTH;
        let i = get_index(id.x, id.y, screen_size);
        storages.states[i][s] += dx[s];
        storages.states[i][s] = clamp(storages.states[i][s], -1., 1.);
    
        
    }

    // display rgba channels
    // let i = id.x + id.y * SCREEN_WIDTH;
    let i = get_index(id.x, id.y, screen_size);
    let xrgb = vec4(storages.states[i][0], storages.states[i][1], storages.states[i][2], storages.states[i][3]) + .5;

    textureStore(
        outputTexture,
        vec2i(id.xy),
        xrgb
    );
}