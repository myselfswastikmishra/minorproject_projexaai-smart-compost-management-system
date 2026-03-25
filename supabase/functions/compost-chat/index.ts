import { serve } from "https://deno.land/std@0.168.0/http/server.ts";

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers": "authorization, x-client-info, apikey, content-type, x-supabase-client-platform, x-supabase-client-platform-version, x-supabase-client-runtime, x-supabase-client-runtime-version",
};

const SYSTEM_PROMPT = `You are CompostIQ, an expert AI assistant specialized in composting and bioWaste management. You help users optimize their composting process using sensor data (PM2.5, temperature, humidity from ESP32 with DHT11 and PM2.5 sensors).

Key knowledge:
- Optimal composting temperature: 55-65°C (thermophilic phase)
- Ideal humidity: 50-60%
- PM2.5 indicates decomposition activity and air quality around the compost
- MQ-135 detects gases like NH3, NOx, CO2, benzene — useful for monitoring decomposition gases and air quality
- Normal composting MQ-135 range: 100-500 ppm; above 500 ppm may indicate anaerobic conditions
- Carbon-to-nitrogen ratio should be ~30:1
- Composting phases: mesophilic (20-40°C), thermophilic (40-70°C), cooling, maturation

When analyzing sensor data, always:
1. State whether conditions are suitable or unsuitable
2. If unsuitable, give specific actionable steps
3. If suitable, estimate time to completion based on the phase
4. Reference the sensor values in your response`;

const ANALYZE_PROMPT = (data: { pm25: number; mq135: number; temperature: number; humidity: number }) =>
  `Analyze these compost sensor readings and determine if conditions are suitable for bioWaste to turn into compost:

- PM2.5: ${data.pm25} µg/m³
- MQ-135 (gas): ${data.mq135} ppm
- Temperature: ${data.temperature}°C
- Humidity: ${data.humidity}%

Provide:
1. Whether current conditions are suitable for composting
2. If unsuitable: specific steps to reach optimal conditions
3. If suitable: estimated timeframe until bioWaste converts to compost
4. Current composting phase based on temperature
5. Gas level assessment from MQ-135 (normal decomposition vs anaerobic concerns)
6. Any concerns about the readings

Format with clear headers and bullet points.`;

serve(async (req) => {
  if (req.method === "OPTIONS") return new Response(null, { headers: corsHeaders });

  try {
    const { messages, type, sensorData } = await req.json();
    const LOVABLE_API_KEY = Deno.env.get("LOVABLE_API_KEY");
    if (!LOVABLE_API_KEY) throw new Error("LOVABLE_API_KEY is not configured");

    let aiMessages: { role: string; content: string }[] = [
      { role: "system", content: SYSTEM_PROMPT },
    ];

    if (type === "analyze" && sensorData) {
      aiMessages.push({ role: "user", content: ANALYZE_PROMPT(sensorData) });
    } else if (type === "chat" && messages) {
      aiMessages = [{ role: "system", content: SYSTEM_PROMPT }, ...messages];
    } else {
      throw new Error("Invalid request type");
    }

    const response = await fetch("https://ai.gateway.lovable.dev/v1/chat/completions", {
      method: "POST",
      headers: {
        Authorization: `Bearer ${LOVABLE_API_KEY}`,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        model: "google/gemini-3-flash-preview",
        messages: aiMessages,
      }),
    });

    if (!response.ok) {
      if (response.status === 429) {
        return new Response(JSON.stringify({ error: "Rate limited. Please try again shortly." }), {
          status: 429,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        });
      }
      if (response.status === 402) {
        return new Response(JSON.stringify({ error: "AI credits exhausted. Please add funds." }), {
          status: 402,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        });
      }
      const text = await response.text();
      console.error("AI error:", response.status, text);
      throw new Error("AI gateway error");
    }

    const data = await response.json();
    const content = data.choices?.[0]?.message?.content || "Unable to analyze at this time.";

    return new Response(JSON.stringify({ response: content }), {
      headers: { ...corsHeaders, "Content-Type": "application/json" },
    });
  } catch (e) {
    console.error("Error:", e);
    return new Response(JSON.stringify({ error: e instanceof Error ? e.message : "Unknown error" }), {
      status: 500,
      headers: { ...corsHeaders, "Content-Type": "application/json" },
    });
  }
});
