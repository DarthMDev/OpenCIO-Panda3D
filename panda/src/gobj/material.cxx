/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file material.cxx
 * @author mike
 * @date 1997-01-09
 */

#include "pandabase.h"
#include "material.h"
#include "indent.h"
#include "datagram.h"
#include "datagramIterator.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "texturePool.h"
#include "filename.h"
#include "stl_compares.h"

TypeHandle Material::_type_handle;
PT(Material) Material::_default;

/**
 *
 */
void Material::
operator = (const Material &copy) {
  Namable::operator = (copy);

  if (is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }

  _base_color = copy._base_color;
  _ambient = copy._ambient;
  _diffuse = copy._diffuse;
  _specular = copy._specular;
  _emission = copy._emission;
  _shininess = copy._shininess;
  _roughness = copy._roughness;
  _metallic = copy._metallic;
  _rim_color = copy._rim_color;
  _rim_width = copy._rim_width;
  _refractive_index = copy._refractive_index;
  _lightwarp_texture = copy._lightwarp_texture;
  _shade_model = copy._shade_model;
  _flags = (copy._flags & ~(F_attrib_lock | F_used_by_auto_shader)) | (_flags & (F_attrib_lock | F_used_by_auto_shader));
}

/**
 * Specifies the base color of the material.  In conjunction with
 * set_metallic, this is an alternate way to specify the color of a material.
 * For dielectrics, this will determine the value of the diffuse color, and
 * for metals, this will determine the value of the specular color.
 *
 * Setting this will clear an explicit specular, diffuse or ambient color
 * assignment.
 *
 * If this is not set, the object color will be used.
 */
void Material::
set_base_color(const LColor &color) {
  if (!has_base_color() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _base_color = color;
  _flags |= F_base_color | F_metallic;
  _flags &= ~(F_ambient | F_diffuse | F_specular);

  // Recalculate the diffuse and specular colors.
  _ambient = _base_color;
  _diffuse = _base_color * (1 - _metallic);

  PN_stdfloat f0 = 0;
  if (_refractive_index >= 1) {
    f0 = (_refractive_index - 1) / (_refractive_index + 1);
    f0 *= f0;
    f0 *= (1 - _metallic);
  }
  _specular.set(f0, f0, f0, 0);
  _specular += _base_color * _metallic;
}

/**
 * Removes the explicit base_color color from the material.
 */
void Material::
clear_base_color() {
  if (has_base_color() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _flags &= ~F_base_color;
  _base_color.set(0.0f, 0.0f, 0.0f, 0.0f);

  if ((_flags & F_ambient) == 0) {
    _ambient.set(0, 0, 0, 0);
  }
  if ((_flags & F_diffuse) == 0) {
    _diffuse.set(0, 0, 0, 0);
  }
  if ((_flags & F_specular) == 0) {
    // Recalculate the specular color.
    PN_stdfloat f0 = 0;
    if (_refractive_index >= 1) {
      f0 = (_refractive_index - 1) / (_refractive_index + 1);
      f0 *= f0;
    }
    _specular.set(f0, f0, f0, 0);
  }
}

/**
 * Specifies the ambient color setting of the material.  This will be the
 * multiplied by any ambient lights in effect on the material to set its base
 * color.
 *
 * This is the color of the object as it appears in the absence of direct
 * light.
 *
 * If this is not set, the object color will be used.
 */
void Material::
set_ambient(const LColor &color) {
  if (!has_ambient() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _ambient = color;
  _flags |= F_ambient;
}

/**
 * Specifies the diffuse color setting of the material.  This will be
 * multiplied by any lights in effect on the material to get the color in the
 * parts of the object illuminated by the lights.
 *
 * This is the primary color of an object; the color of the object as it
 * appears in direct light, in the absence of highlights.
 *
 * If this is not set, the object color will be used.
 */
void Material::
set_diffuse(const LColor &color) {
  if (!has_diffuse() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _diffuse = color;
  _flags |= F_diffuse;
}

/**
 * Specifies the specular color setting of the material.  This will be
 * multiplied by any lights in effect on the material to compute the color of
 * specular highlights on the object.
 *
 * This is the highlight color of an object: the color of small highlight
 * reflections.
 *
 * If this is not set, the specular color is taken from the index of
 * refraction, which is 1 by default (meaning no specular reflections are
 * generated).
 */
void Material::
set_specular(const LColor &color) {
  if (!has_specular() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _specular = color;
  _flags |= F_specular;
}

/**
 * Removes the explicit specular color from the material.
 */
void Material::
clear_specular() {
  if (has_specular() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _flags &= ~F_specular;

  // Recalculate the specular color from the refractive index.
  PN_stdfloat f0 = 0;
  if (_refractive_index >= 1) {
    f0 = (_refractive_index - 1) / (_refractive_index + 1);
    f0 *= f0;
    f0 *= (1 - _metallic);
  }
  _specular.set(f0, f0, f0, 0);
  _specular += _base_color * _metallic;
}

/**
 * Specifies the emission color setting of the material.  This is the color of
 * the object as it appears in the absence of any light whatsover, including
 * ambient light.  It is as if the object is glowing by this color (although
 * of course it will not illuminate neighboring objects).
 *
 * If this is not set, the object will not glow by its own light and will only
 * appear visible in the presence of one or more lights.
 */
void Material::
set_emission(const LColor &color) {
  if (!has_emission() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _emission = color;
  _flags |= F_emission;
}

/**
 * Sets the shininess exponent of the material.  This controls the size of the
 * specular highlight spot.  In general, larger number produce a smaller
 * specular highlight, which makes the object appear shinier.  Smaller numbers
 * produce a larger highlight, which makes the object appear less shiny.
 *
 * This is usually in the range 0..128.
 *
 * Setting a shininess value removes any previous roughness assignment.
 */
void Material::
set_shininess(PN_stdfloat shininess) {
  _shininess = shininess;
  _flags &= ~F_roughness;
}

/**
 * Returns the roughness previously specified by set_roughness.  If none was
 * previously set, this value is computed from the shininess value.
 */
PN_stdfloat Material::
get_roughness() const {
  if ((_flags & F_roughness) == 0) {
    // Calculate roughness from blinn-phong shininess.
    return csqrt(csqrt(2 / (_shininess + 2)));
  } else {
    return _roughness;
  }
}

/**
 * Sets the roughness exponent of the material, where 0 is completely shiny
 * (infinite shininess), and 1 is a completely dull object (0 shininess).
 * This is a different, more perceptually intuitive way of controlling the
 * size of the specular spot, and more commonly used in physically-based
 * rendering.
 *
 * Setting a roughness recalculates the shininess value.
 */
void Material::
set_roughness(PN_stdfloat roughness) {
  _roughness = roughness;
  _flags |= F_roughness;

  // Calculate the specular exponent from the roughness as it is used in
  // Blinn-Phong shading model.  We use the popular Disney method of squaring
  // the roughness to get a more perceptually linear scale.  From:
  // http://graphicrants.blogspot.de/2013/08/specular-brdf-reference.html
  if (roughness <= 0 || IS_NEARLY_ZERO(roughness)) {
    _shininess = make_inf((PN_stdfloat)0);
  } else {
    PN_stdfloat alpha = roughness * roughness;
    _shininess = 2 / (alpha * alpha) - 2;
  }
}

/**
 * Sets the metallic setting of the material, which is is used for physically-
 * based rendering models.  This is usually 0 for dielectric materials and 1
 * for metals.  It really does not make sense to set this to a value other
 * than 0 or 1, but it is nonetheless a float for compatibility with tools
 * that allow setting this to values other than 0 or 1.
 */
void Material::
set_metallic(PN_stdfloat metallic) {
  _metallic = metallic;
  _flags |= F_metallic;

  // Recalculate the diffuse and specular.
  if ((_flags & F_diffuse) == 0) {
    _diffuse = _base_color * (1 - _metallic);
  }
  if ((_flags & F_specular) == 0) {
    // Recalculate the specular color.
    PN_stdfloat f0 = 0;
    if (_refractive_index >= 1) {
      f0 = (_refractive_index - 1) / (_refractive_index + 1);
      f0 *= f0;
      f0 *= (1 - _metallic);
    }
    _specular.set(f0, f0, f0, 0);
    _specular += _base_color * _metallic;
  }
}

/**
 * Removes the explicit metallic setting from the material.
 */
void Material::
clear_metallic() {
  _flags &= ~F_metallic;
  _metallic = 0;

  // If we had a base color, recalculate the diffuse and specular.
  if (_flags & F_base_color) {
    if ((_flags & F_diffuse) == 0) {
      _diffuse = _base_color;
    }
    if ((_flags & F_specular) == 0) {
      // Recalculate the specular color.
      PN_stdfloat f0 = 0;
      if (_refractive_index >= 1) {
        f0 = (_refractive_index - 1) / (_refractive_index + 1);
        f0 *= f0;
      }
      _specular.set(f0, f0, f0, 0);
    }
  }
}

void Material::
set_rim_color(const LColor &color) {
  if (!has_rim_color() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _rim_color = color;
  _flags |= F_rim_color;
}

void Material::
set_rim_width(PN_stdfloat width) {
  if (!has_rim_width() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _rim_width = width;
  _flags |= F_rim_width;
}

void Material::
set_lightwarp_texture(PT(Texture) tex) {
  if (!has_lightwarp_texture() && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _lightwarp_texture = tex;
  _flags |= F_lightwarp_texture;
}

void Material::
set_shade_model(int model) {
  if ((!has_shade_model() || _shade_model != model) && is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
  _shade_model = model;
  _flags |= F_shade_model;
}

/**
 * Sets the index of refraction of the material, which is used to determine
 * the specular color in absence of an explicit specular color assignment.
 * This is usually 1.5 for dielectric materials.  It is not very useful for
 * metals, since they cannot be described as easily with a single number.
 *
 * Should be 1 or higher.  The default is 1.
 */
void Material::
set_refractive_index(PN_stdfloat refractive_index) {
  _refractive_index = refractive_index;
  _flags |= F_refractive_index;

  if ((_flags & F_specular) == 0) {
    // Recalculate the specular color.
    PN_stdfloat f0 = 0;
    if (_refractive_index >= 1) {
      f0 = (_refractive_index - 1) / (_refractive_index + 1);
      f0 *= f0;
    }
    _specular.set(f0, f0, f0, 0);
  }
}

size_t Material::
get_hash_impl() const {
  size_t hash = 0;
  
  hash = int_hash::add_hash(hash, (int)_flags);
  hash = get_base_color().add_hash(hash);
  hash = get_ambient().add_hash(hash);
  hash = get_diffuse().add_hash(hash);
  hash = get_specular().add_hash(hash);
  hash = get_emission().add_hash(hash);
  hash = float_hash().add_hash(hash, get_shininess());
  hash = float_hash().add_hash(hash, get_metallic());
  hash = float_hash().add_hash(hash, get_refractive_index());
  hash = get_rim_color().add_hash(hash);
  hash = float_hash().add_hash(hash, get_rim_width());
  hash = pointer_hash::add_hash(hash, get_lightwarp_texture());
  hash = string_hash::add_hash(hash, get_name());
  
  return hash;
}

/**
 * Returns a number less than zero if this material sorts before the other
 * one, greater than zero if it sorts after, or zero if they are equivalent.
 * The sorting order is arbitrary and largely meaningless, except to
 * differentiate different materials.
 */
int Material::
compare_to(const Material &other) const {
  TypeHandle type = get_type();
  TypeHandle other_type = other.get_type();
  if (type != other_type) {
    return type.get_index() - other_type.get_index();
  }

  // We only call compare_to_impl() if they have the same type.
  return compare_to_impl(&other);
}

int Material::
compare_to_impl(const Material *other) const {
  if (_flags != other->_flags) {
    return _flags - other->_flags;
  }
  if (has_base_color() && get_base_color() != other->get_base_color()) {
    return get_base_color().compare_to(other->get_base_color());
  }
  if (has_ambient() && get_ambient() != other->get_ambient()) {
    return get_ambient().compare_to(other->get_ambient());
  }
  if (has_diffuse() && get_diffuse() != other->get_diffuse()) {
    return get_diffuse().compare_to(other->get_diffuse());
  }
  if (has_specular() && get_specular() != other->get_specular()) {
    return get_specular().compare_to(other->get_specular());
  }
  if (has_emission() && get_emission() != other->get_emission()) {
    return get_emission().compare_to(other->get_emission());
  }
  if (get_shininess() != other->get_shininess()) {
    return get_shininess() < other->get_shininess() ? -1 : 1;
  }
  if (get_metallic() != other->get_metallic()) {
    return get_metallic() < other->get_metallic() ? -1 : 1;
  }
  if (get_refractive_index() != other->get_refractive_index()) {
    return get_refractive_index() < other->get_refractive_index() ? -1 : 1;
  }
  if (has_rim_color() && get_rim_color() != other->get_rim_color()) {
    return get_rim_color().compare_to(other->get_rim_color());
  }
  if (has_rim_width() && get_rim_width() != other->get_rim_width()) {
    return get_rim_width() < other->get_rim_width() ? -1 : 1; 
  }
  if (has_lightwarp_texture() && get_lightwarp_texture() != other->get_lightwarp_texture()) {
    return get_lightwarp_texture() < other->get_lightwarp_texture() ? -1 : 1;
  }

  return strcmp(get_name().c_str(), other->get_name().c_str());
}

/**
 *
 */
void Material::
output(std::ostream &out) const {
  out << "Material " << get_name();
  if (has_base_color()) {
    out << " c(" << get_base_color() << ")";
  } else {
    if (has_ambient()) {
      out << " a(" << get_ambient() << ")";
    }
    if (has_diffuse()) {
      out << " d(" << get_diffuse() << ")";
    }
    if (has_specular()) {
      out << " s(" << get_specular() << ")";
    }
  }
  if (has_refractive_index()) {
    out << " ior" << get_refractive_index();
  }
  if (has_emission()) {
    out << " e(" << get_emission() << ")";
  }
  if (_flags & F_roughness) {
    out << " r" << get_roughness();
  } else {
    out << " s" << get_shininess();
  }
  if (_flags & F_metallic) {
    out << " m" << _metallic;
  }
  if (_flags & F_rim_color) {
    out << " rc(" << get_rim_color() << ")";
  }
  if (_flags & F_rim_width) {
    out << " rw" << get_rim_width();
  }
  if (_flags & F_lightwarp_texture) {
    out << " lwt" << get_lightwarp_texture();
  }
  out << " l" << get_local()
      << " t" << get_twoside();
}

/**
 *
 */
void Material::
write(std::ostream &out, int indent_level) const {
  indent(out, indent_level) << "Material " << get_name() << "\n";
  if (has_base_color()) {
    indent(out, indent_level + 2) << "base_color = " << get_base_color() << "\n";
  }
  if (has_ambient()) {
    indent(out, indent_level + 2) << "ambient = " << get_ambient() << "\n";
  }
  if (has_diffuse()) {
    indent(out, indent_level + 2) << "diffuse = " << get_diffuse() << "\n";
  }
  if (has_specular()) {
    indent(out, indent_level + 2) << "specular = " << get_specular() << "\n";
  } else {
    indent(out, indent_level + 2) << "refractive_index = " << get_refractive_index() << "\n";
  }
  if (has_emission()) {
    indent(out, indent_level + 2) << "emission = " << get_emission() << "\n";
  }
  if (_flags & F_roughness) {
    indent(out, indent_level + 2) << "roughness = " << get_roughness() << "\n";
  } else {
    indent(out, indent_level + 2) << "shininess = " << get_shininess() << "\n";
  }
  if (has_metallic()) {
    indent(out, indent_level + 2) << "metallic = " << get_metallic() << "\n";
  }
  if (has_rim_color()) {
    indent(out, indent_level + 2) << "rim_color = " << get_rim_color() << "\n";
  }
  if (has_rim_width()) {
    indent(out, indent_level + 2) << "rim_width = " << get_rim_width() << "\n";
  }
  if (has_lightwarp_texture()) {
    indent(out, indent_level + 2) << "lightwarp_texture = " << get_lightwarp_texture() << "\n";
  }
  if (has_shade_model()) {
    indent(out, indent_level + 2) << "shade_model = " << get_shade_model() << "\n";
  }
  indent(out, indent_level + 2) << "local = " << get_local() << "\n";
  indent(out, indent_level + 2) << "twoside = " << get_twoside() << "\n";
}



/**
 * Factory method to generate a Material object
 */
void Material::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

/**
 * Function to write the important information in the particular object to a
 * Datagram
 */
void Material::
write_datagram(BamWriter *manager, Datagram &me) {
  me.add_string(get_name());

  if (manager->get_file_minor_ver() >= 39) {
    me.add_int32(_flags & ~F_used_by_auto_shader);

    if (_flags & F_metallic) {
      // Metalness workflow.
      _base_color.write_datagram(me);
      me.add_stdfloat(_metallic);
    } else {
      _ambient.write_datagram(me);
      _diffuse.write_datagram(me);
      _specular.write_datagram(me);
    }
    _emission.write_datagram(me);
    if (_flags & F_rim_color) {
      _rim_color.write_datagram(me);
    }

    if (_flags & F_roughness) {
      me.add_stdfloat(_roughness);
    } else {
      me.add_stdfloat(_shininess);
    }

    me.add_stdfloat(_refractive_index);
    if (_flags & F_rim_width) {
      me.add_stdfloat(_rim_width);
    }
    if (_flags & F_lightwarp_texture) {
      me.add_string(get_lightwarp_texture()->get_fullpath().get_fullpath());
    }
    if (_flags & F_shade_model) {
      me.add_uint8(_shade_model);
    }
  } else {
    _ambient.write_datagram(me);
    _diffuse.write_datagram(me);
    _specular.write_datagram(me);
    _emission.write_datagram(me);
    if (_flags & F_rim_color) {
      _rim_color.write_datagram(me);
    }
    if (_flags & F_rim_width) {
      me.add_stdfloat(_rim_width);
    }
    if (_flags & F_lightwarp_texture) {
      me.add_string(get_lightwarp_texture()->get_fullpath().get_fullpath());
    }
    me.add_stdfloat(_shininess);
    me.add_int32(_flags & 0x7f);
    if (_flags & F_shade_model) {
      me.add_uint8(_shade_model);
    }
    
  }
}

/**
 * Factory method to generate a Material object
 */
TypedWritable *Material::
make_from_bam(const FactoryParams &params) {
  Material *me = new Material;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

/**
 * Function that reads out of the datagram (or asks manager to read) all of
 * the data that is needed to re-create this object and stores it in the
 * appropiate place
 */
void Material::
fillin(DatagramIterator &scan, BamReader *manager) {
  set_name(scan.get_string());

  if (manager->get_file_minor_ver() >= 39) {
    _flags = scan.get_int32();

    if (_flags & F_metallic) {
      // Metalness workflow: read base color and metallic
      _base_color.read_datagram(scan);
      set_metallic(scan.get_stdfloat());

    } else {
      _ambient.read_datagram(scan);
      _diffuse.read_datagram(scan);
      _specular.read_datagram(scan);
    }
    _emission.read_datagram(scan);
    if (_flags & F_rim_color) {
      _rim_color.read_datagram(scan);
    }

    if (_flags & F_roughness) {
      set_roughness(scan.get_stdfloat());
    } else {
      _shininess = scan.get_stdfloat();
    }
    _refractive_index = scan.get_stdfloat();
    if (_flags & F_rim_width) {
      _rim_width = scan.get_stdfloat();
    }
    if (_flags & F_lightwarp_texture) {
      _lightwarp_texture = TexturePool::load_texture(Filename(scan.get_string()));
      _lightwarp_texture->set_wrap_u(SamplerState::WM_clamp);
      _lightwarp_texture->set_wrap_v(SamplerState::WM_clamp);
    }
    if (_flags & F_shade_model) {
      _shade_model = scan.get_uint8();
    }

    if ((_flags & (F_base_color | F_metallic)) == (F_base_color | F_metallic)) {
      // Compute the ambient, diffuse and specular settings.
      set_base_color(_base_color);
    }

  } else {
    _ambient.read_datagram(scan);
    _diffuse.read_datagram(scan);
    _specular.read_datagram(scan);
    _emission.read_datagram(scan);
    if (_flags & F_rim_color) {
      _rim_color.read_datagram(scan);
    }
    if (_flags & F_rim_width) {
      _rim_width = scan.get_stdfloat();
    }
    if (_flags & F_lightwarp_texture) {
      _lightwarp_texture = TexturePool::load_texture(Filename(scan.get_string()));
      _lightwarp_texture->set_wrap_u(SamplerState::WM_clamp);
      _lightwarp_texture->set_wrap_v(SamplerState::WM_clamp);
    }
    _shininess = scan.get_stdfloat();
    _flags = scan.get_int32();

    if (_flags & F_roughness) {
      // The shininess we read is actually a roughness value.
      set_roughness(_shininess);
    }

    if (_flags & F_shade_model) {
      _shade_model = scan.get_uint8();
    }
  }

  if (is_used_by_auto_shader()) {
    GraphicsStateGuardianBase::mark_rehash_generated_shaders();
  }
}
