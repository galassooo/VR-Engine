#pragma once
#include <memory>
#include <unordered_map>

/**
 * @class PhysicsManager
 * @brief Gestisce la simulazione fisica
 *
 * Implementa il pattern Singleton per fornire un accesso centralizzato al sistema fisico.
 * Gestisce collisioni, gravità e altre proprietà fisiche degli oggetti nella scena.
 * Nasconde i dettagli dell'implementazione di Bullet Physics.
 */
class ENG_API PhysicsManager {
public:
    static PhysicsManager& getInstance();
    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager&) = delete;

    /**
     * @brief Struttura per le opzioni di debug fisico
     */
    struct DebugOptions {
        bool drawWireframe = true;           ///< Visualizza wireframe degli oggetti
        bool drawContactPoints = true;       ///< Visualizza i punti di contatto
        bool drawConstraints = true;         ///< Visualizza i vincoli
        glm::vec3 wireframeColor = { 0,1,0 };  ///< Colore wireframe (verde)
        glm::vec3 contactPointColor = { 1,0,0 }; ///< Colore punti di contatto (rosso)
    };

    /**
     * @brief Inizializza il sistema fisico
     * @return True se l'inizializzazione ha successo
     */
    bool initialize();

    /**
     * @brief Rilascia le risorse del sistema fisico
     */
    void cleanup();

    /**
     * @brief Aggiorna la simulazione fisica
     * @param deltaTime Tempo trascorso dall'ultimo aggiornamento (secondi)
     */
    void update(float deltaTime);

    /**
     * @brief Crea un componente fisico per un nodo
     * @param node Nodo a cui aggiungere la fisica
     * @return Puntatore al componente fisico creato
     */
    std::shared_ptr<PhysicsComponent> createComponent(std::shared_ptr<Eng::Node> node);

    /**
     * @brief Rimuove un componente fisico da un nodo
     * @param node Nodo da cui rimuovere la fisica
     * @return True se il componente è stato rimosso con successo
     */
    bool removeComponent(std::shared_ptr<Eng::Node> node);

    /**
     * @brief Ottiene il componente fisico associato a un nodo
     * @param node Nodo di cui ottenere il componente
     * @return Puntatore al componente fisico, nullptr se non esiste
     */
    std::shared_ptr<PhysicsComponent> getComponent(std::shared_ptr<Eng::Node> node);

    /**
     * @brief Crea un collider statico per la scacchiera
     * @param min Punto minimo del box di collisione
     * @param max Punto massimo del box di collisione
     * @return Puntatore al componente fisico creato
     */
    std::shared_ptr<PhysicsComponent> createChessBoardCollider(const glm::vec3& min, const glm::vec3& max);

    /**
     * @brief Imposta la gravità globale
     * @param gravity Vettore gravità (default: {0, -9.81, 0})
     */
    void setGravity(const glm::vec3& gravity);

    /**
     * @brief Ottiene la gravità globale corrente
     * @return Vettore gravità
     */
    glm::vec3 getGravity() const;

    /**
     * @brief Abilita/disabilita il debug della fisica
     * @param enabled True per abilitare, false per disabilitare
     */
    void setDebugMode(bool enabled);

    /**
     * @brief Controlla se il debug è abilitato
     * @return True se il debug è abilitato
     */
    bool isDebugEnabled() const { return debugEnabled; }

    /**
     * @brief Imposta le opzioni di debug
     * @param options Opzioni di debug
     */
    void setDebugOptions(const DebugOptions& options);

    /**
     * @brief Ottiene le opzioni di debug correnti
     * @return Opzioni di debug
     */
    const DebugOptions& getDebugOptions() const { return debugOptions; }

    /**
     * @brief Disegna gli elementi di debug nella scena
     */
    void debugDrawWorld();

    /**
     * @brief Esegue ray casting nella scena fisica
     * @param from Punto di partenza del raggio
     * @param to Punto di arrivo del raggio
     * @param hitPoint Se non nullptr, conterrà il punto di collisione
     * @param hitNormal Se non nullptr, conterrà la normale al punto di collisione
     * @return Nodo colpito dal raggio, nullptr se nessun oggetto è stato colpito
     */
    std::shared_ptr<Eng::Node> rayTest(const glm::vec3& from, const glm::vec3& to,
        glm::vec3* hitPoint = nullptr,
        glm::vec3* hitNormal = nullptr);

private:
    PhysicsManager() = default;
    ~PhysicsManager();

    // Componenti Bullet Physics (opachi all'esterno)
    btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
    btCollisionDispatcher* dispatcher = nullptr;
    btBroadphaseInterface* overlappingPairCache = nullptr;
    btSequentialImpulseConstraintSolver* solver = nullptr;
    btDiscreteDynamicsWorld* dynamicsWorld = nullptr;

    // Mappa i nodi ai componenti fisici
    std::unordered_map<std::shared_ptr<Eng::Node>, std::shared_ptr<PhysicsComponent>> components;

    // Debug
    bool debugEnabled = false;
    DebugOptions debugOptions;
    void* debugDrawer = nullptr;  // Opaco all'esterno

    // Registra un componente nella mappa
    void registerComponent(std::shared_ptr<Eng::Node> node, std::shared_ptr<PhysicsComponent> component);
};