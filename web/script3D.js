import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { STLLoader } from "three/addons/loaders/STLLoader.js";

let container;
let scene;
let camera;
let renderer;
let model;
let controls;
const gridHelper = new THREE.GridHelper(10, 10, 0x444444, 0x888888);
  

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
  camera.position.set(3, 3, 2);
  camera.rotation.set(0, 0, 0);

  controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.05;
  controls.autoRotate = false;

  const ambient = new THREE.AmbientLight(0xffffff, 1.6);
  scene.add(ambient);

  const light = new THREE.DirectionalLight(0xffffff, 1.2);
  light.position.set(10, 10, 100);
  scene.add(light);

  const light2 = new THREE.DirectionalLight(0xffffff, 1.2);
  light2.position.set(-10, -10, -100);
  scene.add(light2);

  const light3 = new THREE.DirectionalLight(0xffffff, 1.2);
  light3.position.set(0, 0, -100);
  scene.add(light3);

  const axesHelper = new THREE.AxesHelper(2);
  scene.add(axesHelper);

  gridHelper.rotation.set(0, 0, 0);
  scene.add(gridHelper);


  const stlLoader = new STLLoader();
  const stlPath = "models/mostro.stl";

  stlLoader.load(
    stlPath,
    (geometry) => {
      geometry.computeVertexNormals();

      const material = new THREE.MeshStandardMaterial({
        color: 0xffa500, // orange
        roughness: 0.4,
        metalness: 0.3,
      });

      model = new THREE.Mesh(geometry, material);

      geometry.computeBoundingBox();
      const box = geometry.boundingBox;
      const center = new THREE.Vector3();
      box.getCenter(center);
      geometry.translate(-center.x, -center.y, -center.z);

      model.scale.setScalar(0.025); 
      model.position.set(0, 0, 0);
      model.rotation.set(0, 0, 0);
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

function initBLEplane(x, y, z) {
  // scene.remove(gridHelper);
  // gridHelper.rotation.set(x, y, z);
  // scene.add(gridHelper);

  // // Center the camera in x, y, z
  // camera.position.set(2, 3, 0);
  // camera.rotation.set(x, y, z);
  // scene.add(camera);
}

function animate() {
  
  requestAnimationFrame(animate);

  renderer.render(scene, camera);
}

let initialGyroSet = false;
let initialGyro = { x: 0, y: 0, z: 0 };

function update3DObject(gx, gy, gz) 
{
  if (!model) 
  {
    console.warn("3D model not loaded yet!");
    return;
  }

  if (!initialGyroSet) 
  {
    initialGyro.x = gx;
    initialGyro.y = gy;
    initialGyro.z = gz;
    initialGyroSet = true;
  }

  const adjustedX = gx - initialGyro.x;
  const adjustedY = gy - initialGyro.y;
  const adjustedZ = gz - initialGyro.z;

  model.rotation.set(-adjustedX, adjustedY, adjustedZ);
}

window.update3DObject = update3DObject;
window.initBLEplane = initBLEplane;

init3DObject();
