import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { STLLoader } from "three/addons/loaders/STLLoader.js";

let container;
let scene;
let camera;
let renderer;
let model;
let controls;

function init3DObject() {
  container = document.getElementById("object");

  container.style.position = "relative";
  container.style.overflow = "hidden";

  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(window.devicePixelRatio);
  renderer.setSize(container.clientWidth, container.clientHeight);
  container.appendChild(renderer.domElement);

  scene = new THREE.Scene();

  camera = new THREE.PerspectiveCamera(
    60,
    container.clientWidth / container.clientHeight,
    0.1,
    1000,
  );
  camera.position.z = 3;
  camera.position.y = 0;
  camera.position.x = 0;

  controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.05;
  controls.autoRotate = false;

  const ambient = new THREE.AmbientLight(0xffffff, 0.6);
  scene.add(ambient);

  const light = new THREE.DirectionalLight(0xffffff, 1.2);
  light.position.set(10, 10, 100);
  scene.add(light);

  const centerGeometry = new THREE.SphereGeometry(0.1, 32, 32);
  const centerMaterial = new THREE.MeshStandardMaterial({
    color: 0xff6b35,
    emissive: 0xff6b35,
    emissiveIntensity: 0.8,
  });
  const centerPoint = new THREE.Mesh(centerGeometry, centerMaterial);
  centerPoint.position.set(0, 0, 0);
  scene.add(centerPoint);

  const axesHelper = new THREE.AxesHelper(2);
  scene.add(axesHelper);

  const gridHelper = new THREE.GridHelper(10, 10, 0x444444, 0x888888);
  scene.add(gridHelper);

  // Load an STL model (place your .stl inside your web folder and update this path)
  const stlLoader = new STLLoader();
  const stlPath = "models/mostro.stl"; // <-- change to your STL path (e.g. "hand.stl" or "assets/hand.stl")

  stlLoader.load(
    stlPath,
    (geometry) => {
      // STL is usually in mm and often has no normals; fix both
      geometry.computeVertexNormals();

      const material = new THREE.MeshStandardMaterial({
        color: 0xfffdd0,
        roughness: 0.4,
        metalness: 0.3,
      });

      model = new THREE.Mesh(geometry, material);

      // Center the STL so rotations happen around its center
      geometry.computeBoundingBox();
      const box = geometry.boundingBox;
      const center = new THREE.Vector3();
      box.getCenter(center);
      geometry.translate(-center.x, -center.y, -center.z);

      // Reasonable default scale (tweak as needed)
      model.scale.setScalar(0.01);
      model.position.set(0, 0, 0);
      scene.add(model);
    },
    undefined,
    (err) => {
      console.error("Failed to load STL:", stlPath, err);
    },
  );

  animate();

  window.addEventListener("resize", () => {
    const w = container.clientWidth;
    const h = container.clientHeight;

    renderer.setSize(w, h);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
  });
}

function animate() {
  requestAnimationFrame(animate);

  // if (model) {
  //     update3DObject(
  //         model.rotation.x + 0.005,
  //         model.rotation.y + 0.01,
  //         model.rotation.z + 0.003
  //     );
  // }

  if (controls) controls.update();
  renderer.render(scene, camera);
}
const targetQ = new THREE.Quaternion();
const filteredEuler = new THREE.Euler(); // store smoothed angles

function update3DObject(roll,pitch) {
  /* if (!model) return;

  targetQ.set(x, y, z, w);

  // Convert quaternion to Euler
  const euler = new THREE.Euler();
  euler.setFromQuaternion(targetQ, "XYZ");

  // Keep yaw fixed
  euler.z = 0;

  // Low-pass filter smoothing
  const alpha = 0.1; // 0.05–0.2 works well
  filteredEuler.x = filteredEuler.x + alpha * (euler.x - filteredEuler.x);
  filteredEuler.y = filteredEuler.y + alpha * (euler.y - filteredEuler.y);
  filteredEuler.z = 0; // keep yaw locked

  // Apply to model
  model.rotation.x = filteredEuler.x;
  model.rotation.y = filteredEuler.y;
  model.rotation.z = filteredEuler.z; */
  if (!model) return;


  // Apply directly to cube (ignore yaw)
  model.rotation.x = THREE.MathUtils.degToRad(roll); // note: x = pitch
  model.rotation.y = THREE.MathUtils.degToRad(pitch);  // note: y = roll
  model.rotation.z = 0;     // yaw locked
}

window.update3DObject = update3DObject;


init3DObject();
