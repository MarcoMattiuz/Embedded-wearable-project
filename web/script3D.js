import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

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

  const geometry = new THREE.BoxGeometry(1.5, 1.5, 1.5);
  const material = new THREE.MeshStandardMaterial({
    color: 0xfffdd0,
    roughness: 0.4,
    metalness: 0.3,
  });
  model = new THREE.Mesh(geometry, material);
  model.position.set(0, 0, 0);
  scene.add(model);

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

  renderer.render(scene, camera);
}

function update3DObject(gx, gy, gz) 
{
  if (!model) 
  {
    console.warn("3D model not loaded yet!");
    return;
  }

  model.rotation.set(gx, gy, gz);
}

window.update3DObject = update3DObject;

init3DObject();
