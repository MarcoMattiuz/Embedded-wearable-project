let container, scene, camera, renderer, cube;

function init3DObject() {

    container = document.getElementById("object");

    // Fix per evitare overflow
    container.style.position = "relative";
    container.style.overflow = "hidden";

    // Renderer
    renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(container.clientWidth, container.clientHeight);
    container.appendChild(renderer.domElement);

    // Scene
    scene = new THREE.Scene();
    // scene.background = new THREE.Color(0x111111);

    // Camera
    camera = new THREE.PerspectiveCamera(
        60,
        container.clientWidth / container.clientHeight,
        0.1,
        1000
    );
    camera.position.z = 3;

    // Lights
    const light = new THREE.DirectionalLight(0xffffff, 1.2);
    light.position.set(2, 2, 3);
    scene.add(light);

    // Cube
    const cubeGeometry = new THREE.BoxGeometry(1, 1, 1);
    const cubeMaterial = new THREE.MeshStandardMaterial({
        color: 0x00ff88,
        roughness: 0.4,
        metalness: 0.3
    });
    cube = new THREE.Mesh(cubeGeometry, cubeMaterial);
    scene.add(cube);

    animate();

    // Resize handling
    window.addEventListener("resize", () => {
        const w = container.clientWidth;
        const h = container.clientHeight;

        renderer.setSize(w, h);
        camera.aspect = w / h;
        camera.updateProjectionMatrix();
    });
}

// Cube animation
function animate() {

    requestAnimationFrame(animate);

    renderer.render(scene, camera);
}

export function update3DObject(gx, gy, gz) {

    cube.rotation.set(gx, gy, gz);
}

init3DObject();