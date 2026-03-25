import { Link, useNavigate } from "react-router-dom";
import { Button } from "@/components/ui/button";
import { supabase } from "@/integrations/supabase/client";
import { Leaf, LogOut, Menu, X } from "lucide-react";
import { useState } from "react";
import { toast } from "sonner";

export default function Navbar() {
  const navigate = useNavigate();
  const [mobileOpen, setMobileOpen] = useState(false);

  const handleLogout = async () => {
    await supabase.auth.signOut();
    toast.success("Logged out successfully");
    navigate("/auth");
  };

  return (
    <nav className="sticky top-0 z-50 glass-card border-b px-4 py-3">
      <div className="container mx-auto flex items-center justify-between">
        <Link to="/" className="flex items-center gap-2">
          <div className="gradient-primary rounded-lg p-1.5">
            <Leaf className="h-5 w-5 text-primary-foreground" />
          </div>
          <span className="font-display text-xl font-bold text-foreground">CompostIQ</span>
        </Link>

        {/* Desktop */}
        <div className="hidden md:flex items-center gap-6">
          <Link to="/" className="text-sm font-medium text-muted-foreground hover:text-foreground transition-colors">Dashboard</Link>
          <Link to="/tutorial" className="text-sm font-medium text-muted-foreground hover:text-foreground transition-colors">Guide</Link>
          <Button variant="ghost" size="sm" onClick={handleLogout}>
            <LogOut className="h-4 w-4 mr-1" /> Logout
          </Button>
        </div>

        {/* Mobile toggle */}
        <button className="md:hidden text-foreground" onClick={() => setMobileOpen(!mobileOpen)}>
          {mobileOpen ? <X className="h-5 w-5" /> : <Menu className="h-5 w-5" />}
        </button>
      </div>

      {mobileOpen && (
        <div className="md:hidden mt-3 flex flex-col gap-2 pb-3">
          <Link to="/" className="text-sm font-medium text-muted-foreground hover:text-foreground px-2 py-1" onClick={() => setMobileOpen(false)}>Dashboard</Link>
          <Link to="/tutorial" className="text-sm font-medium text-muted-foreground hover:text-foreground px-2 py-1" onClick={() => setMobileOpen(false)}>Guide</Link>
          <Button variant="ghost" size="sm" className="justify-start" onClick={handleLogout}>
            <LogOut className="h-4 w-4 mr-1" /> Logout
          </Button>
        </div>
      )}
    </nav>
  );
}
