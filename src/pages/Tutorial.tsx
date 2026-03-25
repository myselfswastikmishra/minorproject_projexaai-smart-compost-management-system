import Navbar from "@/components/Navbar";
import ChatbotWidget from "@/components/ChatbotWidget";
import { Leaf, Thermometer, Droplets, Wind, AlertTriangle, CheckCircle2, Package, Sprout, Clock, ShieldCheck, ArrowRight } from "lucide-react";

const stages = [
  {
    title: "1. Collection & Preparation",
    icon: Package,
    color: "text-compost-warm",
    content: "Collect organic bioWaste — fruit peels, vegetable scraps, coffee grounds, eggshells, yard trimmings. Chop materials into small pieces (2-3 inches) for faster decomposition.",
    readings: "Temperature: ambient (20-30°C) | Humidity: 40-50% | PM2.5: baseline",
  },
  {
    title: "2. Mesophilic Phase (Days 1-7)",
    icon: Thermometer,
    color: "text-destructive",
    content: "Mesophilic bacteria begin breaking down easily digestible compounds. Temperature rises to 40°C. Mix green (nitrogen-rich) and brown (carbon-rich) materials in a 3:1 ratio.",
    readings: "Temperature: 25-40°C | Humidity: 50-60% | PM2.5: 20-50 µg/m³",
  },
  {
    title: "3. Thermophilic Phase (Days 7-30)",
    icon: Sprout,
    color: "text-primary",
    content: "Thermophilic bacteria take over at higher temperatures. This phase kills pathogens and weed seeds. Turn the pile every 3-5 days to maintain oxygen flow.",
    readings: "Temperature: 55-70°C | Humidity: 50-60% | PM2.5: 30-80 µg/m³",
  },
  {
    title: "4. Cooling & Maturation (Days 30-90)",
    icon: Clock,
    color: "text-compost-green-light",
    content: "Temperature gradually decreases as materials are consumed. Fungi and macro-organisms appear. The compost darkens and develops an earthy smell.",
    readings: "Temperature: 30-40°C | Humidity: 40-50% | PM2.5: 10-30 µg/m³",
  },
  {
    title: "5. Finished Compost",
    icon: CheckCircle2,
    color: "text-primary",
    content: "Dark brown/black color, earthy smell, crumbly texture. No recognizable food scraps remaining. Ready to use as soil amendment!",
    readings: "Temperature: ambient | Humidity: 30-40% | PM2.5: < 15 µg/m³",
  },
];

const precautions = [
  "Never compost meat, dairy, or oily foods — they attract pests and create odors",
  "Maintain proper moisture — squeeze test: should feel like a wrung-out sponge",
  "Ensure adequate airflow — turn the pile regularly to prevent anaerobic conditions",
  "Keep PM2.5 exposure low — wear a mask when turning large piles",
  "Monitor temperature — sustained temps above 70°C can kill beneficial organisms",
  "Avoid composting diseased plants or chemically-treated materials",
];

const usageTips = [
  { title: "Garden Beds", desc: "Mix 2-3 inches of compost into the top 6 inches of soil before planting" },
  { title: "Lawn Top-Dressing", desc: "Spread a thin ¼-inch layer over your lawn in spring or fall" },
  { title: "Potting Mix", desc: "Blend 1 part compost with 2 parts potting soil for container plants" },
  { title: "Mulch", desc: "Apply a 2-3 inch layer around plants to retain moisture and suppress weeds" },
  { title: "Compost Tea", desc: "Steep compost in water for 24-48 hours, then use as liquid fertilizer" },
  { title: "Storage", desc: "Store cured compost in a dry, covered bin. It remains viable for 6-12 months" },
];

export default function Tutorial() {
  return (
    <div className="min-h-screen bg-background">
      <Navbar />
      <main className="container mx-auto px-4 py-8 max-w-4xl">
        {/* Hero */}
        <div className="text-center mb-12 animate-fade-in">
          <div className="gradient-primary rounded-2xl p-4 inline-flex mb-4">
            <Leaf className="h-10 w-10 text-primary-foreground" />
          </div>
          <h1 className="text-4xl font-display font-bold text-foreground mb-3">Composting Guide</h1>
          <p className="text-lg text-muted-foreground max-w-2xl mx-auto">
            A complete guide to converting bioWaste into nutrient-rich compost using sensor-driven monitoring
          </p>
        </div>

        {/* Composting Stages */}
        <section className="mb-12">
          <h2 className="text-2xl font-display font-bold text-foreground mb-6">Composting Stages</h2>
          <div className="space-y-4">
            {stages.map((stage) => (
              <div key={stage.title} className="sensor-card animate-fade-in">
                <div className="flex items-start gap-3">
                  <div className="mt-1">
                    <stage.icon className={`h-6 w-6 ${stage.color}`} />
                  </div>
                  <div className="flex-1">
                    <h3 className="text-lg font-display font-bold text-foreground mb-2">{stage.title}</h3>
                    <p className="text-foreground/80 mb-3">{stage.content}</p>
                    <div className="bg-muted/50 rounded-lg px-4 py-2 text-sm text-muted-foreground font-mono">
                      📊 {stage.readings}
                    </div>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </section>

        {/* Precautions */}
        <section className="mb-12">
          <h2 className="text-2xl font-display font-bold text-foreground mb-6 flex items-center gap-2">
            <ShieldCheck className="h-6 w-6 text-compost-warm" />
            Safety & Precautions
          </h2>
          <div className="sensor-card">
            <ul className="space-y-3">
              {precautions.map((p, i) => (
                <li key={i} className="flex items-start gap-3">
                  <AlertTriangle className="h-4 w-4 text-compost-warm mt-0.5 shrink-0" />
                  <span className="text-foreground/80">{p}</span>
                </li>
              ))}
            </ul>
          </div>
        </section>

        {/* Using Compost */}
        <section className="mb-12">
          <h2 className="text-2xl font-display font-bold text-foreground mb-6 flex items-center gap-2">
            <Sprout className="h-6 w-6 text-primary" />
            Using Your Finished Compost
          </h2>
          <div className="grid gap-4 sm:grid-cols-2">
            {usageTips.map((tip) => (
              <div key={tip.title} className="sensor-card">
                <h3 className="font-display font-bold text-foreground mb-1 flex items-center gap-2">
                  <ArrowRight className="h-4 w-4 text-primary" />
                  {tip.title}
                </h3>
                <p className="text-sm text-foreground/80">{tip.desc}</p>
              </div>
            ))}
          </div>
        </section>
      </main>
      <ChatbotWidget />
    </div>
  );
}
